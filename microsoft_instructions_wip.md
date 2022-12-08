# Microsoft 365 Integration

## Microsoft 365 Admin Center
- Go to "https://admin.microsoft.com/Adminportal/Home" and sign in with your Microsoft 365 admin account.
- Create a new account, it will work as a booking user:
  - Open sidebar and click "Users" -> "Active users"
  - Now you should be at "https://admin.microsoft.com/Adminportal/Home#/users"
  - Click "+ Add a user"
    - Select a name and an email e.g. "booking@yourdomain.com"
    - Give it a license with Outlook
- If you have not registered a resource mailbox for your rooms, do so now:
  - Open sidebar and click "Resources" -> "Rooms & equipment"
  - Now you should be at "https://admin.microsoft.com/Adminportal/Home#/ResourceMailbox"
  - Click "+ Add resource"
    - Add a name and an email address, concise names are preferred
    - Click "Save"
  - Now click on the newly created room
    - Under "Delegates", click "Edit" and add our booking user

## Azure
- Go to "https://portal.azure.com" and sign in with your Microsoft 365 admin account.
- Search "App registrations" in the top search bar and click on it.
- URL in search bar should be: https://portal.azure.com/#view/Microsoft_AAD_RegisteredApps/ApplicationsListBlade
- Click "+ New registration"
  - Input a name, e.g. "Monad Booking"
  - Supported account types, leave as default: "Accounts in this organizational directory only (Single tenant)"
  - In section "Redirect URI":
    - Select a platform: Public client/native (mobile & desktop)
    - Input redirect uri: http://localhost:3000
- Select "API permissions" in the left sidebar:
  - Click "+ Add a permission"
    - Select "Microsoft Graph"
    - Select "Delegated permissions"
    - Select "OpenId permissions" > "offline_access" and "Calendars" > "Calendars.ReadWrite"
    - Click "Add permissions"
- Select "Overview" in the left sidebar:
  - Copy the line of random characters next to "Application (client) ID" to clipboard
  - Create a file called "microsoft_credentials.txt" in the root folder of this repository
  - Paste the previously copied line into the file and save it
  - Now the text file should only contain the line of random characters and nothing else

## Generate the credentials
- Open this repository in a terminal
- Make sure that "microsoft_credentials.txt" is in the root folder as instructed earlier
- Install dependencies with command `pip install -r ./scripts/requirements.txt`
- Run command `python ./scripts/microsoft_oauth2flow.py`
- Log in with the booking user created earlier
- It creates a file called "microsoft_token.json". It's used when setting up the devices

