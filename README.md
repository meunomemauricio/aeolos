# Aeolus

Arduino PWM Fan Controller.

## Uploading Firmware

This projects use Platform IO. When building or uploading the firmware, it's
important to specify the build environment. This will set the proper
`MODULE_ID` value depending on the expected fan direction.

    pio run -e exhaust -t upload
    pio run -e intake -t upload
