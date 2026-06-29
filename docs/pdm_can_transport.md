# PDM MCU CAN Transport

The PDM MCU CAN backend carries the common MCU protocol over SocketCAN RAW.
It supports both classic CAN and CAN FD. Classic CAN is the default and uses
fragmentation so the normal `PDM_MCU_MAX_TRANSFER_SIZE` limit remains available.

## Device Tree

A usable board node should set the CAN controller and the request/response CAN
IDs explicitly:

```dts
pdm_mcu_can: mcu@3 {
    compatible = "pdm,mcu-can";
    reg = <3>;
    can-controller = <&can1>;
    tx-can-id = <0x321>;
    rx-can-id = <0x322>;
    rx-timeout-ms = <500>;
};
```

Supported properties:

- `can-controller`: phandle to the CAN netdev provider. Preferred.
- `can`: legacy alias for `can-controller`.
- `can-interface` / `can-ifname`: fallback interface name, such as `can0`.
- `tx-can-id`: CAN ID used by Linux for request frames.
- `rx-can-id` or `response-can-id`: CAN ID used by the MCU for response frames.
- `can-id`: legacy request ID; response ID defaults to `can-id + 1`.
- `can-extended-id`: force 29-bit CAN identifiers.
- `can-fd`: enable CAN FD RAW socket mode and CAN FD frames.
- `can-fd-brs`: set CAN FD bit-rate-switch on transmitted frames.
- `rx-timeout-ms`: response timeout. Use at least 500 ms for classic CAN when
  larger responses are expected.

All properties also accept the same `pdm,` prefix.

## Runtime CAN Setup

The CAN netdev must be configured and up before `pdm.ko` probes the CAN node.
The i.MX6ULL rootfs overlay provides `/etc/init.d/S03pdm`, which defaults to:

```sh
CAN_IFACE=can0
CAN_BITRATE=500000
CAN_FD=0
CAN_DBITRATE=2000000
CAN_RESTART_MS=100
```

Override those environment variables in the boot environment if the board uses a
different bus speed. Set `CAN_FD=1` and `CAN_DBITRATE` when the DTS enables
`can-fd`.

## Wire Format

Each CAN payload begins with a 6-byte PDM CAN transport header:

```text
byte0     magic: 0x50
byte1     high nibble: protocol version, low nibble: flags
byte2     transaction token
byte3     sequence number
byte4..5  total message length, big-endian
byte6..n  fragment payload
```

Flags:

- `0x01`: first fragment
- `0x02`: last fragment
- `0x04`: response frame
- `0x08`: remote error response

A request message is the 32-bit big-endian PDM MCU command followed by the
command payload. The MCU response message contains only the response payload
expected by the shared MCU protocol layer. For example, `GET_VERSION` responds
with `struct pdm_mcu_version` bytes, fragmented if needed.

Fragments with a stale token, wrong sequence, wrong CAN ID, wrong magic, or
unsupported protocol version are ignored or rejected. The Linux side filters the
RAW socket to `rx-can-id`, so unrelated bus traffic should not reach the
transport.

## Quick Validation

After boot:

```sh
/etc/init.d/S03pdm status
ip -details link show can0
ls /dev/pdm
```

For bus-level debugging, use `candump can0` from `can-utils`.
