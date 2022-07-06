# NoBooking

- [NoBooking](#nobooking)
  - [Setup](#setup)
    - [Environment](#environment)
    - [Other](#other)
  - [Deployment](#deployment)
    - [Token Generation](#token-generation)
    - [Build and Upload](#build-and-upload)
    - [Device Setup](#device-setup)

## Setup

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
- Google Cloud Platform
  - Project needs to be created
  - Calendar API needs to be enabled in the project
  - OAuth client ID credentials need to be created and exported to file *credentials.json*
- Organization
  - Needs to create an user to handle the booking e.g. *booking@monad.fi*
- Calendar
  - A room and an automatic calendar for the room needs to be created
  - The room calendar needs to be shared with edit access with our user
  - Id of the room calendar is needed for code operation, it can be retrieved from the settings of the calendar

## Deployment

### Token Generation
You need to place the *credentials.json* file in the root directory of this repository for the next script to work. The script automatically opens a web page where you can log in. Log in with your booking user that was mentioned above.

```sh
python ./scripts/oauth2flow.py
```
This generates a *token.json* file that is used by the program to access and edit the calendars. Put it into a safe place because the same token should be used for all devices. Specifically, 25 is the maximum number of simultaneously working tokens, so generating more carelessly will create problems.

### Build and Upload
Connect the M5Paper to your computer with USB and run the command below.
```sh
pio run -t uploadfs && pio run -t upload
```
This should work most of the time, but sometimes `--upload-port` switch is necessary if PIO can't find the correct port automatically.

### Device Setup
When booting for the first time, the device will go to **setup mode**. It opens a WIFI access point where you can connect in order to set up the device. Note that you will lose internet access when connecting to the access point, so take note of your calendar ID beforehand.

Connect to the WIFI corresponding to the SSID shown on the device. Navigate to the shown IP address in your browser and fill in the configuration options shown on the page.

|Option|Description|
|--|--|
|Name| Can be anything, you can use it to differentiate between devices. |
|Language Code| Language of the device, **FI** and **EN** are supported by default. |
|WIFI SSID| SSID of the WIFI where the device should connect. |
|WIFI Password| Password of the WIFI where the device should connect. |
|IANA Time Zone| Your time zone. |
|Auto Update| Enable if you want the device to check for new updates on every boot up and update automatically. <!-- TODO: add mention of manual updates when ready -->|
|Awake Times| The device saves battery by shutting down at night, choose a time span where the device should be awake. |
|Awake Days| The device saves battery by being shut down in some days of the week (weekends by default), choose the days where the device should be awake. |
|Calendar ID| The ID of the room's calendar. |
|Token.json| The token.json file generated earlier. |

After you have filled in the options and submitted them, restart the device by pressing the reset button in the back and then holding the side button down for a few seconds.

Now configuration is done and the device is ready to use.

