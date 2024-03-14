# ota-detect-update

This application implements Over-The-Air updates of Arduino Nano ESP32 sketch.

A basic Github worflow builds the application upon tag creation toward a matrix of platforms. Compiled binaries are commit in the `main` branch in the `./versions` folder.

The `Build versions file` manual workflow generates a `./versions/list.json` file that holds the available sketch versions.

```json
["v1.0.0","v1.0.1","v1.0.2","v1.1.0","v2.0.0"]
```

Application can list available version by requesting this resource [https://raw.githubusercontent.com/panpanglagla/ota-detect-update/main/versions/list.json](https://raw.githubusercontent.com/panpanglagla/ota-detect-update/main/versions/list.json)

## Build

Install [Arduino CLI](https://github.com/arduino/arduino-cli).

```sh
# Compile sketch
arduino-cli compile  --export-binaries --output-dir ./build/arduino.esp32.nano_nora --fqbn arduino:esp32:nano_nora --config-file="./arduino-cli.yaml" ./

# Upload to USB connected board
arduino-cli upload --fqbn arduino:esp32:nano_nora ./
```

## Resources

- **ArduinoOTA**: [OTASketchDownloadWifi example](https://github.com/JAndrassy/ArduinoOTA/blob/master/examples/Advanced/OTASketchDownloadWifi/OTASketchDownloadWifi.ino)
- **Arduino CLI**: [documentation](https://arduino.github.io/arduino-cli/0.35/)
- [Commit files in a Github action](https://ilikekillnerds.com/2023/01/how-to-create-files-in-a-github-action-and-commit-them-to-your-repository/)
- [Replace string in file](https://stackoverflow.com/questions/525592/find-and-replace-inside-a-text-file-from-a-bash-command) with application version with sed tool
- [AsyncElegantOTA](https://randomnerdtutorials.com/esp32-ota-over-the-air-arduino/) 

