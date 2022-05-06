# M5Paper Room Booking

## Setup

### Environment
```sh
pip install -r requirements.txt
```

### Other
- Google Cloud Platform
  - Project needs to be created
  - Calendar API needs to be enabled
  - OAuth client ID credentials need to be created and exported to file "credentials.json"
- Organization
  - Needs to create an user to handle the booking e.g. booking@monad.fi
- Calendar
  - A room and an automatic calendar for the room needs to be created
  - The room calendar needs to be shared with edit access with our user
  - Id of the room calendar is needed for code operation, it can be retrieved from settings

## Deployment

### Generate token
Authorize the program by logging in with the user:
```sh
python src/oauth2flow.py
```
This generates a token.json that is used by the program to access and edit the calendars.

### Work in progress

