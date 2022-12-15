import json
import os
from google_auth_oauthlib.flow import InstalledAppFlow

TOKEN_FILE = "google_token.json"
CLIENT_FILE = "google_client.json"
SCOPES = ["https://www.googleapis.com/auth/calendar.events"]


def main():
    if not os.path.isfile(CLIENT_FILE):
        print(
            f"Error: File `{CLIENT_FILE}` not found.\n"
            f"    Please export you credentials as json from Google Cloud and rename it to {CLIENT_FILE}."
        )
        return

    flow = InstalledAppFlow.from_client_secrets_file(CLIENT_FILE, SCOPES)
    credentials = flow.run_local_server(port=0)

    token = {
        "refresh_token": credentials.refresh_token,
        "client_id": credentials.client_id,
        "client_secret": credentials.client_secret,
        "scope": credentials.scopes.join(" "),
    }

    with open(TOKEN_FILE, "w") as token:
        token.write(json.dumps(token, indent=2))

    print("All done!")


if __name__ == "__main__":
    main()