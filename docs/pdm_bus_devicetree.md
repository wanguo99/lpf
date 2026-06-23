# PDM Bus Device Tree Notes

PDM Core creates devices from children of a `vendor,pdm-bus` controller node.
Native MCU transports can also create PDM devices from children of UART, I2C,
and SPI controller nodes. Each PDM device appears on `/sys/bus/pdm` and, after
driver binding, one userspace node under `/dev/pdm/`.

Userspace instance numbers are owned by PDM Core. Set `pdm,id` when a stable
`/dev/pdm/<type>N` node is required; otherwise PDM allocates the next free
number for the matched peripheral type.

Supported MCU compatibles:

- `pdm,mcu` or `vendor,pdm-mcu`: generic bring-up device; status/reset only.
- `pdm,mcu-can` or `vendor,pdm-mcu-can`: classic CAN RAW single-frame transport.
- `pdm,mcu-uart` or `vendor,pdm-mcu-uart`: serdev UART transport when the
  node is below a UART controller; legacy file-backed TTY transport when the
  node is below `vendor,pdm-bus` and uses `tty-path`.
- `pdm,mcu-i2c` or `vendor,pdm-mcu-i2c`: I2C client transport below an
  I2C controller.
- `pdm,mcu-spi` or `vendor,pdm-mcu-spi`: SPI client transport below an
  SPI controller.

Supported LED compatibles:

- `pdm,led` or `vendor,pdm-led`: generic bring-up device; in-memory state.
- `pdm,led-gpio` or `vendor,pdm-led-gpio`: GPIO descriptor output.
- `pdm,led-pwm` or `vendor,pdm-led-pwm`: PWM duty-cycle output.

Example:

```dts
pdm_bus: pdm-bus {
    compatible = "vendor,pdm-bus";

    mcu0: mcu@0 {
        compatible = "pdm,mcu-can";
        reg = <0>;
        can-interface = "can0";
        rx-timeout-ms = <100>;
        /* Optional: use extended CAN IDs even for values <= 0x7ff. */
        /* can-extended-id; */
    };

    mcu1: mcu@1 {
        compatible = "pdm,mcu-uart";
        reg = <1>;
        tty-path = "/dev/ttyS1";
        baudrate = <115200>;
        rx-timeout-ms = <100>;
    };

    status_led: led@0 {
        compatible = "pdm,led-gpio";
        reg = <0>;
        led-gpios = <&gpio1 3 GPIO_ACTIVE_HIGH>;
        max-brightness = <1>;
        default-state = "off";
    };

    panel_led: led@1 {
        compatible = "pdm,led-pwm";
        reg = <1>;
        pwms = <&pwm1 0 1000000>;
        max-brightness = <255>;
        default-brightness = <32>;
    };
};
```

MCU-CAN maps `PDM_MCU_IOC_COMMAND.command` and `pdm_mcu_data.address` to the CAN
ID. The current implementation intentionally supports one classic CAN frame per
ioctl, so payloads larger than 8 bytes return `-EMSGSIZE`.

Legacy MCU-UART nodes under `vendor,pdm-bus` use `tty-path` with `filp_open()`
and synchronous `kernel_read()` / `kernel_write()`. This remains available for
compatibility, but new UART MCU descriptions should use serdev by placing the MCU
node below the UART controller:

```dts
&uart1 {
    status = "okay";

    mcu0: mcu {
        compatible = "pdm,mcu-uart";
        pdm,id = <0>;
        current-speed = <115200>;
        rx-timeout-ms = <100>;
        /* Optional: uart-has-rtscts; or flow-control = "hw"; */
    };
};
```

I2C and SPI MCU transports are described below their native Linux controller
nodes. `pdm,id` selects the `/dev/pdm/mcuN` index. If it is omitted, PDM Core
allocates the next free MCU index; the native `reg` property remains the I2C
address or SPI chip select and is not used as a userspace instance number.

```dts
&i2c1 {
    status = "okay";

    mcu1: mcu@10 {
        compatible = "pdm,mcu-i2c";
        reg = <0x10>;
        pdm,id = <1>;
        rx-timeout-ms = <100>;
        command-bytes = <1>;
        address-bytes = <1>;
    };
};

&spi0 {
    status = "okay";

    mcu2: mcu@0 {
        compatible = "pdm,mcu-spi";
        reg = <0>;
        pdm,id = <2>;
        spi-max-frequency = <1000000>;
        rx-timeout-ms = <100>;
        command-bytes = <1>;
        address-bytes = <1>;
    };
};
```

For I2C/SPI, `PDM_MCU_IOC_COMMAND.command` is encoded as a big-endian prefix
before `tx_data`; `PDM_MCU_IOC_READ_DATA.address` and
`PDM_MCU_IOC_WRITE_DATA.address` are encoded as a big-endian address prefix.
`command-bytes` and `address-bytes` default to 1 and may be set to 0 through 4.
