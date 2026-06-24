// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_mcu_can.c
 * @brief CAN RAW transport for PDM MCU devices
 */

#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/err.h>
#include <linux/if.h>
#include <linux/module.h>
#include <linux/net.h>
#include <linux/netdevice.h>
#include <linux/of.h>
#include <linux/socket.h>
#include <net/net_namespace.h>
#include <net/sock.h>

#include "pdm/compat/pdm_compat_features.h"
#include "pdm/core/driver/pdm_backend.h"
#include "pdm_mcu_internal.h"
#include "osal.h"


static const struct of_device_id pdm_mcu_can_of_match[] = {
	{ .compatible = "pdm,mcu-can" },
	{ .compatible = "vendor,pdm-mcu-can" },
	{ }
};
MODULE_DEVICE_TABLE(of, pdm_mcu_can_of_match);

static canid_t pdm_mcu_can_make_id(struct pdm_mcu_instance *inst, u32 id)
{
	if (inst->transport.can.extended_id || id > CAN_SFF_MASK)
		return (id & CAN_EFF_MASK) | CAN_EFF_FLAG;

	return id & CAN_SFF_MASK;
}

static u32 pdm_mcu_can_strip_id(canid_t can_id)
{
	if (can_id & CAN_EFF_FLAG)
		return can_id & CAN_EFF_MASK;

	return can_id & CAN_SFF_MASK;
}

static int pdm_mcu_can_send_frame(struct pdm_mcu_instance *inst, u32 id,
				  const u8 *data, u32 len)
{
	struct can_frame frame = { };
	struct msghdr msg = { };
	struct kvec vec;
	int ret;

	if (!inst->transport.can.sock)
		return -ENODEV;
	if (len > CAN_MAX_DLEN)
		return -EMSGSIZE;

	frame.can_id = pdm_mcu_can_make_id(inst, id);
	frame.len = len;
	if (len)
		memcpy(frame.data, data, len);

	vec.iov_base = &frame;
	vec.iov_len = sizeof(frame);
	ret = kernel_sendmsg(inst->transport.can.sock, &msg, &vec, 1,
			     sizeof(frame));
	if (ret < 0)
		return ret;

	return ret == sizeof(frame) ? 0 : -EIO;
}

static int pdm_mcu_can_recv_frame(struct pdm_mcu_instance *inst, u32 *id,
				  u8 *data, u32 *len)
{
	struct can_frame frame = { };
	struct msghdr msg = { };
	struct kvec vec;
	u32 max_len = *len;
	int ret;

	if (!inst->transport.can.sock)
		return -ENODEV;
	if (max_len > PDM_MCU_MAX_TRANSFER_SIZE)
		return -EMSGSIZE;

	vec.iov_base = &frame;
	vec.iov_len = sizeof(frame);
	ret = kernel_recvmsg(inst->transport.can.sock, &msg, &vec, 1,
			     sizeof(frame), 0);
	if (ret < 0)
		return ret;
	if (ret != sizeof(frame))
		return -EIO;
	if (frame.len > max_len)
		return -EMSGSIZE;

	*id = pdm_mcu_can_strip_id(frame.can_id);
	*len = frame.len;
	if (frame.len)
		memcpy(data, frame.data, frame.len);
	return 0;
}

static int pdm_mcu_can_setup(struct pdm_mcu_instance *inst)
{
	struct device_node *np = inst->pdm_dev->dev.of_node;
	struct net_device *netdev;
	struct sockaddr_can addr = { };
	const char *ifname = "can0";
	u32 value;
	int ret;

	if (np) {
		of_property_read_string(np, "can-interface", &ifname);
		if (!of_property_read_u32(np, "rx-timeout-ms", &value))
			inst->transport.can.rx_timeout_ms = value;
		inst->transport.can.extended_id =
			of_property_read_bool(np, "can-extended-id");
	}
	if (!inst->transport.can.rx_timeout_ms)
		inst->transport.can.rx_timeout_ms = PDM_MCU_DEFAULT_RX_TIMEOUT_MS;
	strscpy(inst->transport.can.ifname, ifname,
		sizeof(inst->transport.can.ifname));

	netdev = dev_get_by_name(&init_net, inst->transport.can.ifname);
	if (!netdev)
		return -ENODEV;

	ret = sock_create_kern(&init_net, PF_CAN, SOCK_RAW, CAN_RAW,
			       &inst->transport.can.sock);
	if (ret)
		goto out_put_netdev;

	if (inst->transport.can.sock->sk)
		inst->transport.can.sock->sk->sk_rcvtimeo =
			msecs_to_jiffies(inst->transport.can.rx_timeout_ms);

	addr.can_family = AF_CAN;
	addr.can_ifindex = netdev->ifindex;
#if PDM_KERNEL_HAS_SOCKADDR_UNSIZED
	ret = kernel_bind(inst->transport.can.sock,
			  (struct sockaddr_unsized *)&addr, sizeof(addr));
#else
	ret = kernel_bind(inst->transport.can.sock, (struct sockaddr *)&addr,
			  sizeof(addr));
#endif
	if (ret)
		goto err_release_sock;

	LOG_INFO("Bound CAN transport to %s",
		 inst->transport.can.ifname);
	dev_put(netdev);
	return 0;

err_release_sock:
	sock_release(inst->transport.can.sock);
	inst->transport.can.sock = NULL;
out_put_netdev:
	dev_put(netdev);
	return ret;
}

static void pdm_mcu_can_cleanup(struct pdm_mcu_instance *inst)
{
	if (!inst->transport.can.sock)
		return;

	sock_release(inst->transport.can.sock);
	inst->transport.can.sock = NULL;
}

static int pdm_mcu_can_reset(struct pdm_mcu_instance *inst)
{
	(void)inst;
	return 0;
}

static int pdm_mcu_can_command(struct pdm_mcu_instance *inst,
			       struct pdm_mcu_command *command)
{
	u32 id;
	u32 rx_len;
	int ret;

	if (command->tx_len > CAN_MAX_DLEN || command->rx_len > CAN_MAX_DLEN)
		return -EMSGSIZE;

	ret = pdm_mcu_can_send_frame(inst, command->command,
				      command->tx_data, command->tx_len);
	if (ret)
		return ret;

	if (!command->rx_len)
		return 0;

	rx_len = command->rx_len;
	ret = pdm_mcu_can_recv_frame(inst, &id, command->rx_data, &rx_len);
	if (ret)
		return ret;
	command->rx_len = rx_len;
	return 0;
}

static int pdm_mcu_can_read_data(struct pdm_mcu_instance *inst,
				 struct pdm_mcu_data *data)
{
	u32 id;
	u32 len;
	int ret;

	if (data->len > PDM_MCU_MAX_TRANSFER_SIZE)
		return -EMSGSIZE;

	len = data->len ? min_t(u32, data->len, CAN_MAX_DLEN) : CAN_MAX_DLEN;
	ret = pdm_mcu_can_recv_frame(inst, &id, data->data, &len);
	if (ret)
		return ret;
	data->address = id;
	data->len = len;
	return 0;
}

static int pdm_mcu_can_write_data(struct pdm_mcu_instance *inst,
				  const struct pdm_mcu_data *data)
{
	if (data->len > CAN_MAX_DLEN)
		return -EMSGSIZE;

	return pdm_mcu_can_send_frame(inst, data->address, data->data, data->len);
}

static const struct pdm_mcu_transport_ops pdm_mcu_can_ops = {
	.type = PDM_MCU_BACKEND_CAN,
	.name = "can",
	.capability = PDM_CTL_DEVICE_CAP_TRANSPORT_CAN,
	.setup = pdm_mcu_can_setup,
	.cleanup = pdm_mcu_can_cleanup,
	.reset = pdm_mcu_can_reset,
	.command = pdm_mcu_can_command,
	.read_data = pdm_mcu_can_read_data,
	.write_data = pdm_mcu_can_write_data,
};

pdm_backend_register(mcu_can, PDM_CTL_DEVICE_TYPE_MCU,
		     PDM_BACKEND_CLASS_TRANSPORT, pdm_mcu_can_of_match,
		     &pdm_mcu_can_ops, NULL, NULL);
