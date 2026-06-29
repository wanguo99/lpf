// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_mcu_can.c
 * @brief CAN RAW transport for PDM MCU devices
 */

#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/if_arp.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/net.h>
#include <linux/netdevice.h>
#include <linux/of.h>
#include <linux/of_net.h>
#include <linux/sched.h>
#include <linux/socket.h>
#include <linux/string.h>
#include <net/net_namespace.h>
#include <net/sock.h>

#include "pdm/compat/pdm_compat_socket.h"
#include "pdm/registry/pdm_backend.h"
#include "pdm/pdm_mcu.h"
#include "pdm_mcu_internal.h"
#include "osal.h"

/*
 * PDM CAN transport frame payload:
 *   byte0: magic
 *   byte1: protocol version in high nibble, flags in low nibble
 *   byte2: transaction token
 *   byte3: sequence number
 *   byte4..5: total message length, big-endian
 *   byte6..n: message fragment
 *
 * The request message starts with a big-endian 32-bit PDM MCU command followed
 * by the command payload. Responses carry PDM_MCU_CAN_FLAG_RESPONSE and contain
 * the raw response payload expected by the shared MCU protocol layer.
 */
#define PDM_MCU_CAN_MAGIC 0x50U
#define PDM_MCU_CAN_VERSION 1U
#define PDM_MCU_CAN_DEFAULT_TX_ID 0x321U
#define PDM_MCU_CAN_DEFAULT_RX_ID 0x322U
#define PDM_MCU_CAN_FRAME_HDR_SIZE 6U
#define PDM_MCU_CAN_MAX_TOKEN 0xffU

#define PDM_MCU_CAN_FLAG_FIRST BIT(0)
#define PDM_MCU_CAN_FLAG_LAST BIT(1)
#define PDM_MCU_CAN_FLAG_RESPONSE BIT(2)
#define PDM_MCU_CAN_FLAG_ERROR BIT(3)

struct pdm_mcu_can_header {
	u8 magic;
	u8 version_flags;
	u8 token;
	u8 seq;
	u8 total_hi;
	u8 total_lo;
} __packed;

static u16 pdm_mcu_can_get_be16(const u8 *buf)
{
	return ((u16)buf[0] << 8) | buf[1];
}

static void pdm_mcu_can_put_be16(u8 *buf, u16 value)
{
	buf[0] = (u8)(value >> 8);
	buf[1] = (u8)value;
}

static u8 pdm_mcu_can_flags(const struct pdm_mcu_can_header *hdr)
{
	return hdr->version_flags & 0x0f;
}

static u32 pdm_mcu_can_payload_limit(const struct pdm_mcu_instance *inst)
{
	return inst->transport.can.frame_data_size - PDM_MCU_CAN_FRAME_HDR_SIZE;
}

static canid_t pdm_mcu_can_format_id(const struct pdm_mcu_instance *inst,
					     u32 id)
{
	if (inst->transport.can.extended_id) {
		return (id & CAN_EFF_MASK) | CAN_EFF_FLAG;
	}

	return id & CAN_SFF_MASK;
}

static u32 pdm_mcu_can_raw_id(const struct pdm_mcu_instance *inst,
				      canid_t id)
{
	return id & (inst->transport.can.extended_id ? CAN_EFF_MASK :
		     CAN_SFF_MASK);
}

static canid_t pdm_mcu_can_filter_mask(const struct pdm_mcu_instance *inst)
{
	return inst->transport.can.extended_id ?
		(CAN_EFF_FLAG | CAN_RTR_FLAG | CAN_EFF_MASK) :
		(CAN_EFF_FLAG | CAN_RTR_FLAG | CAN_SFF_MASK);
}

static u32 pdm_mcu_can_frame_payload_len(const struct pdm_mcu_instance *inst,
						 const struct canfd_frame *frame)
{
	if (inst->transport.can.can_fd) {
		return frame->len;
	}

	return min_t(u8, frame->len, CAN_MAX_DLEN);
}

static int pdm_mcu_can_setsockopt(struct socket *sock, int optname,
					  void *optval, unsigned int optlen)
{
	return pdm_compat_kernel_setsockopt(sock, SOL_CAN_RAW, optname,
					      optval, optlen);
}

static int pdm_mcu_can_configure_socket(struct pdm_mcu_instance *inst)
{
	struct can_filter filter;
	int enabled = 1;
	int disabled = 0;
	int ret;

	if (inst->transport.can.can_fd) {
		ret = pdm_mcu_can_setsockopt(inst->transport.can.sock,
						  CAN_RAW_FD_FRAMES,
						  &enabled, sizeof(enabled));
		if (ret) {
			return ret;
		}
	}

	filter.can_id = inst->transport.can.rx_id;
	filter.can_mask = pdm_mcu_can_filter_mask(inst);
	ret = pdm_mcu_can_setsockopt(inst->transport.can.sock, CAN_RAW_FILTER,
					  &filter, sizeof(filter));
	if (ret) {
		return ret;
	}

	ret = pdm_mcu_can_setsockopt(inst->transport.can.sock,
					  CAN_RAW_RECV_OWN_MSGS,
					  &disabled, sizeof(disabled));
	if (ret) {
		return ret;
	}

	return 0;
}

static int pdm_mcu_can_send_frame(struct pdm_mcu_instance *inst,
					  const struct canfd_frame *frame)
{
	struct msghdr msg = { };
	struct kvec vec;
	size_t frame_size = inst->transport.can.can_fd ? CANFD_MTU : CAN_MTU;
	int ret;

	vec.iov_base = (void *)frame;
	vec.iov_len = frame_size;
	ret = kernel_sendmsg(inst->transport.can.sock, &msg, &vec, 1, frame_size);
	if (ret < 0) {
		return ret;
	}

	return ret == frame_size ? 0 : -EIO;
}

static int pdm_mcu_can_recv_frame(struct pdm_mcu_instance *inst,
					  struct canfd_frame *frame)
{
	struct msghdr msg = { };
	struct kvec vec;
	int ret;

	vec.iov_base = frame;
	vec.iov_len = sizeof(*frame);
	ret = kernel_recvmsg(inst->transport.can.sock, &msg, &vec, 1,
			     sizeof(*frame), 0);
	if (ret < 0) {
		return ret;
	}
	if (ret != CAN_MTU && ret != CANFD_MTU) {
		return -EIO;
	}
	if (!inst->transport.can.can_fd && ret != CAN_MTU) {
		return -EPROTO;
	}
	if (inst->transport.can.can_fd && ret == CANFD_MTU &&
	    frame->len > CANFD_MAX_DLEN) {
		return -EPROTO;
	}
	if (ret == CAN_MTU && frame->len > CAN_MAX_DLEN) {
		return -EPROTO;
	}
	if ((frame->can_id & (CAN_ERR_FLAG | CAN_RTR_FLAG)) ||
	    (frame->can_id & pdm_mcu_can_filter_mask(inst)) !=
	    (inst->transport.can.rx_id & pdm_mcu_can_filter_mask(inst))) {
		return -EAGAIN;
	}

	return 0;
}

static void pdm_mcu_can_fill_header(struct pdm_mcu_can_header *hdr,
					    u8 flags, u8 seq, u8 token, u16 total_len)
{
	hdr->magic = PDM_MCU_CAN_MAGIC;
	hdr->version_flags = ((PDM_MCU_CAN_VERSION & 0x0f) << 4) |
		(flags & 0x0f);
	hdr->token = token;
	hdr->seq = seq;
	pdm_mcu_can_put_be16(&hdr->total_hi, total_len);
}

static int pdm_mcu_can_parse_header(const struct canfd_frame *frame,
					    u32 frame_len,
					    struct pdm_mcu_can_header *hdr)
{
	if (frame_len < sizeof(*hdr)) {
		return -EPROTO;
	}

	memcpy(hdr, frame->data, sizeof(*hdr));
	if (hdr->magic != PDM_MCU_CAN_MAGIC) {
		return -EPROTO;
	}
	if ((hdr->version_flags >> 4) != PDM_MCU_CAN_VERSION) {
		return -EPROTO;
	}

	return 0;
}

static int pdm_mcu_can_send_message(struct pdm_mcu_instance *inst,
					    u8 token, u32 command,
					    const u8 *tx, u32 tx_len)
{
	u8 message[PDM_MCU_MAX_TRANSFER_SIZE + PDM_MCU_TRANSPORT_ID_BYTES];
	u32 message_len = tx_len + PDM_MCU_TRANSPORT_ID_BYTES;
	u32 chunk_limit = pdm_mcu_can_payload_limit(inst);
	u32 offset = 0;
	u8 seq = 0;
	int ret;

	if (tx_len > PDM_MCU_MAX_TRANSFER_SIZE || message_len > sizeof(message)) {
		return -EMSGSIZE;
	}
	if (!chunk_limit || chunk_limit > CANFD_MAX_DLEN) {
		return -EINVAL;
	}

	pdm_mcu_can_put_be16(message, (u16)(command >> 16));
	pdm_mcu_can_put_be16(message + 2, (u16)command);
	if (tx_len) {
		memcpy(message + PDM_MCU_TRANSPORT_ID_BYTES, tx, tx_len);
	}

	do {
		struct canfd_frame frame = { };
		struct pdm_mcu_can_header *hdr;
		u32 left = message_len - offset;
		u32 chunk = min_t(u32, left, chunk_limit);
		u8 flags = 0;

		if (!offset) {
			flags |= PDM_MCU_CAN_FLAG_FIRST;
		}
		if (offset + chunk == message_len) {
			flags |= PDM_MCU_CAN_FLAG_LAST;
		}

		frame.can_id = inst->transport.can.tx_id;
		frame.len = PDM_MCU_CAN_FRAME_HDR_SIZE + chunk;
		if (inst->transport.can.can_fd) {
#ifdef CANFD_FDF
			frame.flags = CANFD_FDF;
#endif
			if (inst->transport.can.can_fd_brs) {
				frame.flags |= CANFD_BRS;
			}
		}

		hdr = (struct pdm_mcu_can_header *)frame.data;
		pdm_mcu_can_fill_header(hdr, flags, seq, token, message_len);
		memcpy(frame.data + PDM_MCU_CAN_FRAME_HDR_SIZE,
		       message + offset, chunk);

		ret = pdm_mcu_can_send_frame(inst, &frame);
		if (ret) {
			return ret;
		}

		offset += chunk;
		seq++;
	} while (offset < message_len);

	return 0;
}

static int pdm_mcu_can_recv_message(struct pdm_mcu_instance *inst,
					    u8 token, u8 *rx, u32 *rx_len)
{
	unsigned long deadline;
	u32 expected_len = 0;
	u32 copied = 0;
	u8 expected_seq = 0;
	bool saw_first = false;

	if (!rx_len) {
		return 0;
	}

	deadline = jiffies + msecs_to_jiffies(inst->transport.can.rx_timeout_ms);
	while (time_before(jiffies, deadline)) {
		struct canfd_frame frame = { };
		struct pdm_mcu_can_header hdr;
		u32 frame_len;
		u32 payload_len;
		u32 total_len;
		u8 flags;
		int ret;

		ret = pdm_mcu_can_recv_frame(inst, &frame);
		if (ret == -EAGAIN || ret == -EPROTO) {
			cond_resched();
			continue;
		}
		if (ret) {
			return ret;
		}

		frame_len = pdm_mcu_can_frame_payload_len(inst, &frame);
		ret = pdm_mcu_can_parse_header(&frame, frame_len, &hdr);
		if (ret) {
			cond_resched();
			continue;
		}

		if (hdr.token != token) {
			cond_resched();
			continue;
		}

		flags = pdm_mcu_can_flags(&hdr);
		if (!(flags & PDM_MCU_CAN_FLAG_RESPONSE)) {
			cond_resched();
			continue;
		}
		if (flags & PDM_MCU_CAN_FLAG_ERROR) {
			return -EREMOTEIO;
		}
		if (hdr.seq != expected_seq) {
			return -EILSEQ;
		}

		payload_len = frame_len - PDM_MCU_CAN_FRAME_HDR_SIZE;
		total_len = pdm_mcu_can_get_be16(&hdr.total_hi);
		if (flags & PDM_MCU_CAN_FLAG_FIRST) {
			if (expected_seq != 0 || saw_first) {
				return -EILSEQ;
			}
			if (total_len > *rx_len ||
			    total_len > PDM_MCU_MAX_TRANSFER_SIZE) {
				return -EMSGSIZE;
			}
			expected_len = total_len;
			saw_first = true;
		} else if (!saw_first) {
			cond_resched();
			continue;
		} else if (total_len != expected_len) {
			return -EILSEQ;
		}

		if (copied + payload_len > expected_len) {
			return -EMSGSIZE;
		}
		if (payload_len) {
			memcpy(rx + copied,
			       frame.data + PDM_MCU_CAN_FRAME_HDR_SIZE,
			       payload_len);
			copied += payload_len;
		}
		expected_seq++;

		if (flags & PDM_MCU_CAN_FLAG_LAST) {
			if (copied != expected_len) {
				return -EIO;
			}
			*rx_len = copied;
			return 0;
		}
	}

	return -ETIMEDOUT;
}

static int pdm_mcu_can_xfer_message(struct pdm_mcu_instance *inst,
					    u32 command, const u8 *tx, u32 tx_len,
					    u8 *rx, u32 *rx_len)
{
	u8 token;
	int ret;

	inst->transport.can.token = (inst->transport.can.token + 1) &
		PDM_MCU_CAN_MAX_TOKEN;
	if (!inst->transport.can.token) {
		inst->transport.can.token = 1;
	}
	token = (u8)inst->transport.can.token;

	ret = pdm_mcu_can_send_message(inst, token, command, tx, tx_len);
	if (ret) {
		return ret;
	}
	if (!rx_len || !*rx_len) {
		return 0;
	}

	return pdm_mcu_can_recv_message(inst, token, rx, rx_len);
}

static bool pdm_mcu_can_read_u32(struct device_node *np,
				     const char *plain_name,
				     const char *pdm_name, u32 *value)
{
	if (!np || !value) {
		return false;
	}
	if (pdm_name && !of_property_read_u32(np, pdm_name, value)) {
		return true;
	}
	if (plain_name && !of_property_read_u32(np, plain_name, value)) {
		return true;
	}

	return false;
}

static bool pdm_mcu_can_read_bool(struct device_node *np,
				      const char *plain_name,
				      const char *pdm_name)
{
	return np && ((pdm_name && of_property_read_bool(np, pdm_name)) ||
		      (plain_name && of_property_read_bool(np, plain_name)));
}

static bool pdm_mcu_can_read_ifname(struct device_node *np, const char **ifname)
{
	if (!np || !ifname) {
		return false;
	}

	return !of_property_read_string(np, "pdm,can-interface", ifname) ||
	       !of_property_read_string(np, "can-interface", ifname) ||
	       !of_property_read_string(np, "pdm,can-ifname", ifname) ||
	       !of_property_read_string(np, "can-ifname", ifname);
}

static struct net_device *pdm_mcu_can_find_netdev(struct device_node *np,
						  bool *missing_config)
{
	struct device_node *can_np;
	struct net_device *netdev;
	const char *ifname;

	if (missing_config) {
		*missing_config = false;
	}
	if (!np) {
		return NULL;
	}

	can_np = of_parse_phandle(np, "can-controller", 0);
	if (!can_np) {
		can_np = of_parse_phandle(np, "can", 0);
	}
	if (can_np) {
		netdev = of_find_net_device_by_node(can_np);
		of_node_put(can_np);
		return netdev;
	}

	if (pdm_mcu_can_read_ifname(np, &ifname)) {
		return dev_get_by_name(&init_net, ifname);
	}

	if (missing_config) {
		*missing_config = true;
	}
	return NULL;
}

static int pdm_mcu_can_configure_ids(struct pdm_mcu_instance *inst,
				     struct device_node *np)
{
	u32 tx_id = PDM_MCU_CAN_DEFAULT_TX_ID;
	u32 rx_id = PDM_MCU_CAN_DEFAULT_RX_ID;

	if (pdm_mcu_can_read_u32(np, "can-id", "pdm,can-id", &tx_id)) {
		rx_id = tx_id + 1;
	}
	pdm_mcu_can_read_u32(np, "tx-can-id", "pdm,tx-can-id", &tx_id);
	if (!pdm_mcu_can_read_u32(np, "rx-can-id", "pdm,rx-can-id",
				      &rx_id)) {
		pdm_mcu_can_read_u32(np, "response-can-id",
				     "pdm,response-can-id", &rx_id);
	}

	if (tx_id > CAN_EFF_MASK || rx_id > CAN_EFF_MASK) {
		return -EINVAL;
	}
	if (tx_id > CAN_SFF_MASK || rx_id > CAN_SFF_MASK) {
		inst->transport.can.extended_id = true;
	}

	inst->transport.can.tx_id = pdm_mcu_can_format_id(inst, tx_id);
	inst->transport.can.rx_id = pdm_mcu_can_format_id(inst, rx_id);
	return 0;
}

static int pdm_mcu_can_setup(struct pdm_mcu_instance *inst)
{
	struct device_node *np = inst->base.pdm_dev->dev.of_node;
	struct sockaddr_can addr = { };
	struct net_device *netdev;
	bool missing_config = false;
	u32 value;
	int ret;

	inst->transport.can.rx_timeout_ms = PDM_MCU_DEFAULT_RX_TIMEOUT_MS;
	inst->transport.can.frame_data_size = CAN_MAX_DLEN;
	if (pdm_mcu_can_read_u32(np, "rx-timeout-ms",
				     "pdm,rx-timeout-ms", &value) && value) {
		inst->transport.can.rx_timeout_ms = value;
	}
	inst->transport.can.extended_id =
		pdm_mcu_can_read_bool(np, "can-extended-id",
				      "pdm,can-extended-id");
	inst->transport.can.can_fd =
		pdm_mcu_can_read_bool(np, "can-fd", "pdm,can-fd");
	inst->transport.can.can_fd_brs =
		pdm_mcu_can_read_bool(np, "can-fd-brs", "pdm,can-fd-brs");
	if (inst->transport.can.can_fd_brs && !inst->transport.can.can_fd) {
		return -EINVAL;
	}
	if (inst->transport.can.can_fd) {
		inst->transport.can.frame_data_size = CANFD_MAX_DLEN;
	}

	ret = pdm_mcu_can_configure_ids(inst, np);
	if (ret) {
		return ret;
	}

	netdev = pdm_mcu_can_find_netdev(np, &missing_config);
	if (!netdev) {
		return missing_config ? -EINVAL : -EPROBE_DEFER;
	}
	if (netdev->type != ARPHRD_CAN) {
		ret = -EINVAL;
		goto err_put_netdev;
	}

	ret = sock_create_kern(&init_net, PF_CAN, SOCK_RAW, CAN_RAW,
			       &inst->transport.can.sock);
	if (ret) {
		goto err_put_netdev;
	}

	if (inst->transport.can.sock->sk) {
		inst->transport.can.sock->sk->sk_rcvtimeo =
			msecs_to_jiffies(inst->transport.can.rx_timeout_ms);
		inst->transport.can.sock->sk->sk_sndtimeo =
			msecs_to_jiffies(inst->transport.can.rx_timeout_ms);
	}

	ret = pdm_mcu_can_configure_socket(inst);
	if (ret) {
		goto err_release_sock;
	}

	addr.can_family = AF_CAN;
	addr.can_ifindex = netdev->ifindex;
	ret = pdm_compat_kernel_bind(inst->transport.can.sock,
				     (struct sockaddr *)&addr, sizeof(addr));
	if (ret) {
		goto err_release_sock;
	}

	inst->transport.can.netdev = netdev;
	LOG_INFO("Bound CAN transport to %s tx=0x%x rx=0x%x %s %s",
		 dev_name(&netdev->dev),
		 pdm_mcu_can_raw_id(inst, inst->transport.can.tx_id),
		 pdm_mcu_can_raw_id(inst, inst->transport.can.rx_id),
		 inst->transport.can.extended_id ? "extended" : "standard",
		 inst->transport.can.can_fd ? "can-fd" : "classic-can");
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

static int pdm_mcu_can_xfer(struct pdm_mcu_instance *inst,
				    struct pdm_mcu_xfer *xfer)
{
	u32 len = xfer->rx_len;
	int ret;

	if (!inst->transport.can.sock) {
		return -ENODEV;
	}

	ret = pdm_mcu_can_xfer_message(inst, xfer->command, xfer->tx,
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
	.max_tx_size = PDM_MCU_MAX_TRANSFER_SIZE,
	.max_rx_size = PDM_MCU_MAX_TRANSFER_SIZE,
	.setup = pdm_mcu_can_setup,
	.cleanup = pdm_mcu_can_cleanup,
	.xfer = pdm_mcu_can_xfer,
};

pdm_backend_register(mcu_can, PDM_MCU_DEVICE_TYPE,
		     PDM_BACKEND_CLASS_TRANSPORT, pdm_mcu_can_of_match,
		     &pdm_mcu_can_ops, NULL, NULL);
