# mbed-os-example-http(s)

This application demonstrates how to make HTTP and HTTPS requests and parse the response from mbed OS 5.

It consists of four demo's, which you can select in ``source/select-demo.h``.

* HTTP demo:
    * Does a GET request to http://httpbin.org/status/418.
    * Does a POST request to http://httpbin.org/post.
* HTTPS demo:
    * Does a GET request to https://developer.mbed.org/media/uploads/mbed_official/hello.txt.
    * Does a POST request to https://httpbin.org/post.
* HTTP demo with socket re-use.
* HTTPS demo with socket re-use.

Response parsing is done through [nodejs/http-parser](https://github.com/nodejs/http-parser).

## To build

1. Open ``mbed_app.json`` and change the `network-interface` option to your connectivity method ([more info](https://github.com/ARMmbed/easy-connect)).
2. Build the project in the online compiler or using mbed CLI.
3. Flash the project to your development board.
4. Attach a serial monitor to your board to see the debug messages.

## Entropy (or lack thereof)

On all platforms **except** the FRDM-K64F and FRDM-K22F the application is compiled without TLS entropy sources. This means that your code is inherently unsafe and should not be deployed to any production systems. To enable entropy, remove the `MBEDTLS_NO_DEFAULT_ENTROPY_SOURCES` and `MBEDTLS_TEST_NULL_ENTROPY` macros from mbed_app.json.

## Tested on

* K64F with Ethernet.
* NUCLEO_F411RE with ESP8266.
