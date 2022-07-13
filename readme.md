# NoBooking

- [NoBooking](#nobooking)
  - [Setup](#setup)
    - [Environment](#environment)
    - [Other](#other)
  - [Deployment](#deployment)
    - [Token Generation](#token-generation)
    - [Build and Upload](#build-and-upload)
    - [Device Setup](#device-setup)
    - [Changing the Configuration](#changing-the-configuration)
  - [Troubleshooting](#troubleshooting)
    - [Boot Errors](#boot-errors)
    - [Runtime Errors](#runtime-errors)

<!-- TODO: Add general information about this project and why it exists -->

## Setup

If you know git, pull this repository with it.

If you don't know git, [download the project as zip](https://github.com/monadoy/m5paper/archive/refs/heads/master.zip) and unzip it. The resulting folder is the repository root folder.

### Environment
Requirements:
- [Python 3](https://www.python.org/downloads/)
- [Node.js 16](https://nodejs.org/en/download/)

After installing the requirements above, open your console in the root folder of this repository and run the commands below to set up the project.
```sh
# Install python libraries
pip install -r ./scripts/requirements.txt

# Setup littlefs and build frontend
python ./scripts/setup.py littlefs frontend
```

### Other
- Google Cloud Platform ([detailed instructions](https://github.com/monadoy/m5paper/wiki/Google-Cloud-Platform-Setup))
  - Project needs to be created
  - Calendar API needs to be enabled in the project
  - OAuth client ID credentials need to be created (Choose *Desktop app*) and exported to file *credentials.json*
- Google Workspace Admin
  - Needs to create a user to handle the booking e.g. *booking@monad.fi*
- Google Calendar
  - A room and an automatic calendar for the room needs to be created
  - The room calendar needs to be shared with *edit access* with our user
  - Id of the room calendar is needed for code operation, it can be retrieved from the settings of the calendar

## Deployment

### Token Generation
You need to place the *credentials.json* file in the root directory of this repository for the next script to work. Run the command below to automatically open a web page where you can log in. Log in with your booking user that was mentioned above.

```sh
python ./scripts/oauth2flow.py
```
This generates a *token.json* file that is used by the program to access and edit the calendars. Put it into a safe place because the same token should be used for all devices. Specifically, 25 is the maximum number of simultaneously working tokens, so generating more carelessly will create problems.

### Build and Upload
Connect the M5Paper to your computer with USB and run the command below.
```sh
pio run -t uploadfs && pio run -t upload
```
This should work most of the time, but sometimes the `--upload-port` option is necessary if PIO can't find the correct port automatically.

### Device Setup
When booting for the first time, the device will go to **setup mode**. It opens a WIFI access point where you can connect in order to set up the device. Note that you will lose internet access when connecting to the access point, so take note of your calendar ID beforehand.

Connect to the WIFI corresponding to the SSID shown on the device. Navigate to the shown IP address in your browser and fill in the configuration options shown on the page.

|Option|Description|
|--|--|
|Name| Can be anything, you can use it to differentiate between devices. |
|Language Code| Language of the device, **FI** and **EN** are supported by default. |
|WIFI SSID| SSID of the WIFI where the device should connect. Only 2.4 GHz frequency is supported. |
|WIFI Password| Password of the WIFI where the device should connect. |
|IANA Time Zone| Your time zone. |
|Auto Update| Enable if you want the device to check for new updates on every boot up and update automatically. <!-- TODO: add mention of manual updates when ready -->|
|Awake Times| The device saves battery by shutting down at night, choose a time span where the device should be awake. To achieve a full week of battery life, the device should be set to shut down for about 12 hours each day. You can experiment with different values. |
|Awake Days| The device saves battery by being shut down in some days of the week (weekends by default), choose the days where the device should be awake. |
|Calendar ID| The ID of the room's calendar. |
|Token.json| The token.json file generated earlier. |

After you have filled in the options and submitted them, restart the device by pressing the reset button in the back and then holding the side button down for a few seconds.

Now configuration is done and the device is ready to use.

### Changing the Configuration

If the device has successfully booted, you can change its configuration. Enter the options menu by pressing the button in the top left of the screen. Then press the *setup* button. Next you have to be connected to the same WIFI that the device is connected to, and navigate to the IP shown on the device in your browser. Change the configuration as needed and reboot the device when done.

Some configuration options like tokens and passwords are hidden to keep them safe. If they already exist on the device, you don't need to fill them in, because the device merges new options with old ones.

## Troubleshooting

### Boot Errors

During the boot sequence, there are a multitude of errors that can occur. The device prints the cause on the screen. Some issues are caused by invalid configuration and some are caused by external factors like spotty internet.

Here are some solutions to common boot errors.
|Error|Solution|
|--|--|
|*WIFI Error*| WIFI is unavailable for some reason. A simple reboot can fix the problem if it was caused by instability in the WIFI access point. Otherwise the device is probably set up incorrectly. Ensure that the WIFI is 2.4 GHz instead of 5 GHz. If it says *NO_AP_FOUND*, the WIFI SSID was probably set up incorrectly. If it says something about timeouts, the WIFI password was probably set up incorrectly. Fix by [rebuilding](#build-and-Upload) and redoing the setup with correct WIFI credentials.
|*Couldn't sync with NTP server.*| Time synchronization server are unavailable for some reason. Usually there is nothing we can do except retry.
|*Token parse failed*| Token was most likely copy pasted incorrectly or left completely blank. Fix by [rebuilding](#build-and-Upload) and redoing the setup with the correct token. |

### Runtime Errors

Some errors manifest after booting. These appear in the top middle of the screen. Most are caused by badly behaving WIFI, but some are caused by incorrect configuration.


Here are some solutions to common runtime errors.
|Error|Solution|
|--|--|
|*HTTP: 404, not_found*| This likely means that the the calendar ID is wrong. [Change the configuration](#changing-the-configuration) as needed. |
|*HTTP: 401, unauthorized*| This means that the booking user doesn't have the correct permissions. Make sure to give the user [that you logged in with](#token-generation) edit access to the calendar. |
|*HTTP: connection refused*| This happens when WIFI is unavailable. Likely requires no action as WIFI can be unreliable at times. |

<!--

FIXME: Custom localization won't actually work, because updates will overwrite the customized localization.json

## Localization
You can add your preferred language to the device. Languages using the latin alphabet should work easily. Other alphabets and right to left writing most likely won't work.

Tranlations are stored under the *data* directory in the *localization.json* file. There you should add your language code to the *supportedLanguages* list. After that you can add your translations in the same way as "FI" and "EN" items are added already.
-->




