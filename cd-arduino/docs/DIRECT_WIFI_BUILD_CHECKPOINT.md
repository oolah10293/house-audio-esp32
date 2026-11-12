# Direct home Wi-Fi source node checkpoint — 2026-11-11

The user explicitly authorized a new XIAO ESP32-S3 Arduino source build that **removes the temporary Wi-Fi setup network**. These changes remain on the `feature/reusable-input-node` development branch and are not deployed to the user's hardware or production Pi.

- Uses `WIFI_STA` exclusively, joining the user's existing home Wi-Fi on boot.
- Removed `House-CD-Setup`, saved Preferences Wi-Fi configuration, and `POST /configure`.
- Reads SSID and password at compile time from private local `LocalNetworkConfig.h`.
- Committed only `LocalNetworkConfig.example.h` containing placeholders; the real file is ignored by Git.
- Default Pi gateway: `192.168.1.12:8768`; source identity and CD audio HTTP transport unchanged.
- Status page on node port 80 reports its Wi-Fi IP and last Pi registration HTTP status.
- Recovery: if USB host firmware hides the COM port, hold XIAO BOOT while plugging it into PC, then upload with the previously proven Arduino IDE 2.0.14 + `--no-stub` setup.
- CI copies the example to local config header before compiling for `esp32:esp32:XIAO_ESP32S3` on exact core 2.0.14.
- Tested locally: ten Python tests passed, including static network layout. Arduino CI result must be checked separately; **CD audio streaming and network registration are not yet verified on physical hardware**.
- Existing Pi gateway on port 8768 and working House Music service remain untouched.

The user-facing source ZIP includes a placeholder `LocalNetworkConfig.h` ready for the user to edit with their private SSID/password before uploading.
