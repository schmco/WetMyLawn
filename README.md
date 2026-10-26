# WET_MY_LAWN
This repository tracks the code of WET_MY_LAWN (WML). The target of the WML is to implement an irrigation system based on
an ESP32, featuring automated or manual control of solenoid valves, weather data integration, and support for sensors (rainfall, soil moisture).

Based on the data for each sprinkler zone (soil type, area, lawn or flower bed, etc., and sprinkler flow rate), an irrigation duration is calculated for each zone. Combined with the sensor data and the weather forecast, the system can then water according to the irrigation schedule or adjust its strategy.

## Directory structure
The directory is structured as follows: 

| Directory    | Description                                                  |
|--------------|--------------------------------------------------------------|
| build/       | Build output (object files + executable)                     |
| docs/        | Documentation (e.g., coding guidelines, images)              |
| main/        | app_main for ESP_IDF applikation (.c/.h)                     |
| components/  | contains folders with components like drivers                |
| components/name  | contains .c file and CMakeLists for the component _name_ |
| components/name/include  | contains .h file for the component _name_        |


## Hardware 
At the moment, the hardware target is an eval board, an out-of-the-box solution available at [Amazon](https://amzn.eu/d/0hWrT7BF). It features four SRD-05VDC-SL-C relays and is based on an ESP32-32E N4 chip. It is programmed with the ESP-Prog or a similar device.

## Build
_Todo_.

## CMake
_Todo_.

## Tests
_Todo_.

## Pushing a new change
The following steps should be performed for each change. 

1. Create a local branch
2. Make the code changes
3. Build the code
4. Flash and test the code on the target
5. Static analyse the code
6. Format the code
7. Commit the code
8. Push the branch to GitHub
9. Open a pull-request
10. Merge the pull request

## Commit message
The commit message should follow the [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/) specification and should therefore be written according to the following scheme: 

```Example.
<type>[optional scope]: <description>

[optional body]

[optional footer(s)]
```

## Code formatter
For the purpose of auto formatting the code, **clang-format** is used. **clang-format** is available within the _LLVM_ compiler project. You can install the complete collection with the provided installer [LLVM](https://releases.llvm.org/) or just download the installer, extract clang-format and add it to your system path variable.
The _Makefile_ provides a phonie target for **clang-format**, which can be executed with the following bash command:
```Bash
make format
```
The formating rules are defined in _.clangformat_ provided in the root directory of this project.

## Coding guidlines
_Todo_

## Static analysis
_Todo_.

## Memory footprint analysis
_Todo_.

## Assert
_Todo_.

## Software architecture
_Todo_

## State machine
_Todo_

## Schematic
_Todo_