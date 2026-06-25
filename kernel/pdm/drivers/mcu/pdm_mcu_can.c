// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_mcu_can.c
 * @brief CAN RAW transport for PDM MCU devices
 */

#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/net.h>
#include <linux/netdevice.h>
#include <linux/of.h>
#include <linux/of_net.h>
#include <linux/socket.h>
#include <linux/string.h>
#include <net/net_namespace.h>
#include <net/sock.h>

#include "pdm/compat/pdm_compat_features.h"
#include "pdm/registry/pdm_backend.h"
#include "pdm/pdm_mcu.h"
#include "pdm_mcu_internal.h"
#include "osal.h"

static canid_t pdm_mcu_can_make_id(struct pdm_mcu_instance *inst, u32 id)
{
	if (inst->transport.can.extended_id || id > CAN_SFF_MASK) {
		return (id & CAN_EFF_MASK) | CAN_EFF_FLAG;
	}

	return id & CAN_SFF_MASK;
}

static u32 pdm_mcu_can_strip_id(canid_t can_id)
{
	if (can_id & CAN_EFF_FLAG) {
		return can_id & CAN_EFF_MASK;
	}

	return can_id & CAN_SFF_MASK;
}

static int pdm_mcu_can_send_frame(struct pdm_mcu_instance *inst, u32 id,
				  const u8 *data, u32 len)
{
	struct can_frame frame = { };
	struct msghdr msg = { };
	struct kvec vec;
	int ret;

	if (!inst->transport.can.sock) {
		return -ENODEV;
	}
	if (len > CAN_MAX_DLEN) {
		return -EMSGSIZE;
	}

	frame.can_id = pdm_mcu_can_make_id(inst, id);
	frame.len = len;
	if (len) {
		memcpy(frame.data, data, len);
	}

	vec.iov_base = &frame;
	vec.iov_len = sizeof(frame);
	ret = kernel_sendmsg(inst->transport.can.sock, &msg, &vec, 1,
			     sizeof(frame));
	if (ret < 0) {
		return ret;
	}

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

	if (!inst->transport.can.sock) {
		return -ENODEV;
	}
	if (max_len > PDM_MCU_MAX_TRANSFER_SIZE) {
		return -EMSGSIZE;
	}

	vec.iov_base = &frame;
	vec.iov_len = sizeof(frame);
	ret = kernel_recvmsg(inst->transport.can.sock, &msg, &vec, 1,
			     sizeof(frame), 0);
	if (ret < 0) {
		return ret;
	}
	if (ret != sizeof(frame)) {
		return -EIO;
	}
	if (frame.len > max_len) {
		return -EMSGSIZE;
	}

	*id = pdm_mcu_can_strip_id(frame.can_id);
	*len = frame.len;
	if (frame.len) {
		memcpy(data, frame.data, frame.len);
	}
	return 0;
}

static struct net_device *pdm_mcu_can_find_netdev(struct device_node *np)
{
	struct device_node *can_np;
	struct net_device *netdev;

	if (!np) {
		return NULL;
	}

	can_np = of_parse_phandle(np, "can-controller", 0);
	if (!can_np) {
		can_np = of_parse_phandle(np, "can", 0);
	}
	if (!can_np) {
		return NULL;
	}

	netdev = of_find_net_device_by_node(can_np);
	of_node_put(can_np);
	return netdev;
}

static int pdm_mcu_can_setup(struct pdm_mcu_instance *inst)
{
	struct device_node *np = inst->base.pdm_dev->dev.of_node;
	struct sockaddr_can addr = { };
	struct net_device *netdev;
	u32 value;
	int ret;

	netdev = pdm_mcu_can_find_netdev(np);
	if (!netdev) {
		return -ENODEV;
	}

	inst->transport.can.rx_timeout_ms = PDM_MCU_DEFAULT_RX_TIMEOUT_MS;
	if (np) {
		if (!of_property_read_u32(np, "rx-timeout-ms", &value)) {
			inst->transport.can.rx_timeout_ms = value;
		}
		inst->transport.can.extended_id =
			of_property_read_bool(np, "can-extended-id");
	}

	ret = sock_create_kern(&init_net, PF_CAN, SOCK_RAW, CAN_RAW,
			       &inst->transport.can.sock);
	if (ret) {
		goto err_put_netdev;
	}

	if (inst->transport.can.sock->sk) {
		inst->transport.can.sock->sk->sk_rcvtimeo =
			msecs_to_jiffies(inst->transport.can.rx_timeout_ms);
	}

	addr.can_family = AF_CAN;
	addr.can_ifindex = netdev->ifindex;
#if PDM_KERNEL_HAS_SOCKADDR_UNSIZED
	ret = kernel_bind(inst->transport.can.sock,
			  (struct sockaddr_unsized *)&addr, sizeof(addr));
#else
	ret = kernel_bind(inst->transport.can.sock, (struct sockaddr *)&addr,
			  sizeof(addr));
#endif
	if (ret) {
		goto err_release_sock;
	}

	inst->transport.can.netdev = netdev;
	LOG_INFO("Bound CAN transport to %s", dev_name(&netdev->dev));
	return 0;

err_release_sock:
	sock_release(inst->transport.can.sock);
	inst->transport.can.sock = NULL;
err_put_netdev:
	dev_put(netdev);
	return ret;
}

static void pdm_mcu_can_cleanup(struct pdm_mcu_instance *inst)
{
	if (inst->transport.can.sock) {
		sock_release(inst->transport.can.sock);
		inst->transport.can.sock = NULL;
	}
	if (inst->transport.can.netdev) {
		dev_put(inst->transport.can.netdev);
		inst->transport.can.netdev = NULL;
	}
}

static int pdm_mcu_can_cmd_xfer(struct pdm_mcu_instance *inst, u32 command,
				const u8 *tx, u32 tx_len, u8 *rx, u32 *rx_len)
{
	int ret;

	ret = pdm_mcu_can_send_frame(inst, command, tx, tx_len);
	if (ret) {
		return ret;
	}
	if (!rx_len || !*rx_len) {
		return 0;
	}

	return pdm_mcu_can_recv_frame(inst, &command, rx, rx_len);
}

static int pdm_mcu_can_xfer(struct pdm_mcu_instance *inst,
			    struct pdm_mcu_xfer *xfer)
{
	u32 len = xfer->rx_len;
	int ret;

	ret = pdm_mcu_can_cmd_xfer(inst, xfer->command, xfer->tx,
				   xfer->tx_len, xfer->rx, &len);
	if (ret) {
		return ret;
	}

	xfer->actual_rx_len = len;
	return 0;
}

static const struct of_device_id pdm_mcu_can_of_match[] = {
	{ .compatible = "pdm,mcu-can" },
	{ .compatible = "vendor,pdm-mcu-can" },
	{ }
};
MODULE_DEVICE_TABLE(of, pdm_mcu_can_of_match);

static const struct pdm_mcu_transport_ops pdm_mcu_can_ops = {
	.name = "can",
	.capability = PDM_MCU_CAP_TRANSPORT_CAN,
	.max_tx_size = CAN_MAX_DLEN,
	.max_rx_size = CAN_MAX_DLEN,
	.setup = pdm_mcu_can_setup,
	.cleanup = pdm_mcu_can_cleanup,
	.xfer = pdm_mcu_can_xfer,
};

pdm_backend_register(mcu_can, PDM_MCU_DEVICE_TYPE,
		     PDM_BACKEND_CLASS_TRANSPORT, pdm_mcu_can_of_match,
		     &pdm_mcu_can_ops, NULL, NULL);
