import webbrowser
from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import urlparse, parse_qs
import random
import string
import requests
import json
from datetime import datetime
import time


def randStr(chars=string.ascii_uppercase + string.digits, N=10):
    return ''.join(random.choice(chars) for _ in range(N))


CLIENT_FILE = "microsoft_client.txt"
REDIRECT_URI = "http://localhost:3000"
SCOPE = "offline_access https://graph.microsoft.com/calendars.readwrite https://graph.microsoft.com/calendars.readwrite.shared"
STATE = randStr(N=20)

code = None


class OAuthHandler(BaseHTTPRequestHandler):

    def do_GET(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()

        # Read code and state from the URL
        url = urlparse(self.path)
        queries = parse_qs(url.query)
        try:
            global code
            code = queries["code"][0]
            if queries["state"][0] != STATE:
                raise Exception("State mismatch")
        except Exception as e:
            self.wfile.write(f"Authentication failed. Try again.\n"
                             f"Reason: {repr(e)}".encode())
            return

        self.wfile.write(
            "Authentication successful. You can close this window now.".encode(
            ))


def main():
    try:
        with open(CLIENT_FILE, "r") as f:
            client_id = f.read().strip()
    except FileNotFoundError:
        print(f"Error: File `{CLIENT_FILE}` not found.\n"
              f"    Please create a file called {CLIENT_FILE} and put "
              "your client ID in it.")
        return

    webbrowser.open(
        f"https://login.microsoftonline.com/organizations/oauth2/v2.0/authorize"
        f"?client_id={client_id}"
        f"&redirect_uri={REDIRECT_URI}"
        f"&scope={SCOPE}"
        f"&state={STATE}"
        f"&response_type=code"
        f"&response_mode=query")

    # Start a local HTTP server to receive the auth code
    print("Starting a local server for OAuth callback..."
          "(Press Ctrl+C to abort)")
    server_address = ("", 3000)
    httpd = HTTPServer(server_address, OAuthHandler)
    httpd.handle_request()

    if code is None:
        print("Authentication failed.")
        return

    print("Authentication success, fetching refresh token...")

    # Exchange auth code for refresh token
    response = requests.post(
        "https://login.microsoftonline.com/organizations/oauth2/v2.0/token",
        data={
            "client_id": client_id,
            "scope": SCOPE,
            "code": code,
            "redirect_uri": REDIRECT_URI,
            "grant_type": "authorization_code"
        })

    if response.status_code != 200:
        print(f"Failed to fetch refresh token. Reason: {response.text}")
        return

    data = response.json()

    expiry = datetime.fromtimestamp((time.time()) + data["expires_in"])
    token = {
        "refresh_token": data["refresh_token"],
        "client_id": client_id,
        "scope": data["scope"],
    }

    with open("microsoft_token.json", "w") as f:
        f.write(json.dumps(token, indent=2))
    
    print("All done!")


if __name__ == "__main__":
    main()