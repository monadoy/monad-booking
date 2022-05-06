import datetime
import os.path

from google.auth.transport.requests import Request
from google.oauth2.credentials import Credentials
from googleapiclient.discovery import build
from googleapiclient.errors import HttpError

TOKEN_FILE = "token.json"
CLIENT_SECRETS_FILE = "credentials.json"
RESOURCE_CALENDAR_ID = "c_188d3eihprds8jhujiij785n116ke@resource.calendar.google.com"
SCOPES = ["https://www.googleapis.com/auth/calendar"]

BOOKING_SUMMARY = "M5paper Booking"
BOOKING_DESCRIPTION = "Booking made with m5paper"


def get_current_and_next_event(service):
    """
    Fetches events from the Google Calendar API service.
    Returns (current_event, next_event), both can be None.
    Can throw HttpError.
    """
    now = datetime.datetime.utcnow()
    now_str = now.isoformat() + "Z"

    # FIXME: This isn't actually midnight in local timezone
    midnight_str = (now.replace(hour=0, minute=0, second=0, microsecond=0) +
                    datetime.timedelta(days=1)).isoformat() + "Z"

    events_result = service.events().list(calendarId=RESOURCE_CALENDAR_ID,
                                          timeMin=now_str,
                                          timeMax=midnight_str,
                                          maxResults=2,
                                          singleEvents=True,
                                          orderBy="startTime").execute()

    events = events_result.get("items", [])

    for event in events:
        # Whole day events contain a date, ignore these for now
        if event["start"].get("date"):
            return (None, None)

    cur_event = None
    next_event = None

    # Find out if first returned event is current or not
    if len(events) >= 1:
        start_str = events[0]["start"].get("dateTime")
        start = datetime.datetime.fromisoformat(start_str)

        # FIXME: Doesn't work when event contains non UTC timestamps
        if start <= now:
            cur_event = events[0]
        else:
            next_event = events[0]

    # If first event was current, set second event as next
    if len(events) == 2 and next_event == None:
        next_event = events[1]

    return (cur_event, next_event)


def insert_event_starting_now(service, duration_minutes):
    """
    Inserts a new event.
    If insertion overlaps with existing events, the event will be created but immediately declined.
     -> one has to check beforehand that no overlap happens
    Minutes are rounded up to make the event ending time divisible by 5.
    Returns the new event or None.
    Can throw HttpError.
    """

    now = datetime.datetime.utcnow()

    start_time_str = now.isoformat() + "Z"

    # NOTE: This calculation could probably happen outside of this function
    # to make it easier to check for overlaps
    if now.minute % 5 > 0:
        minutes_delta = duration_minutes + 5 - now.minute % 5
    else:
        minutes_delta = duration_minutes
    end_time_str = (
        now + datetime.timedelta(minutes=minutes_delta)).isoformat() + "Z"
    # ============================

    eventBody = {
        "summary": BOOKING_SUMMARY,
        "description": BOOKING_DESCRIPTION,
        "start": {
            "dateTime": start_time_str
        },
        "end": {
            "dateTime": end_time_str
        },
        "attendees": [{
            "email": RESOURCE_CALENDAR_ID
        }]
    }

    event = service.events().insert(calendarId=RESOURCE_CALENDAR_ID,
                                    body=eventBody).execute()

    if not event:
        print("Could not insert event.")
        return None

    return event


def end_event(service, event_id):
    """
    Ends the event associated with event_id now.
    Returns the new event or None.
    Can throw HttpError.
    """
    now = datetime.datetime.utcnow()
    end_time_str = now.isoformat() + "Z"

    # set any event values to be changed
    eventBodyChanges = {
        "end": {
            "dateTime": end_time_str
        },
    }

    event = service.events().patch(calendarId=RESOURCE_CALENDAR_ID,
                                   eventId=event_id,
                                   body=eventBodyChanges).execute()

    if not event:
        print("Could not patch event.")
        return None

    return event


def list_events_example(service):
    """
    Reference: https://developers.google.com/calendar/api/v3/reference/events/list
    Notice: 
    - returns events that that end after timeMin, meaning that half way done events are returned too
    """
    now_str = datetime.datetime.utcnow().isoformat() + "Z"
    events_result = service.events().list(
        calendarId=RESOURCE_CALENDAR_ID,
        timeMin=now_str,
        # timeMax
        maxResults=10,
        singleEvents=True,  # Return recurring events as singular instances
        orderBy="startTime").execute()
    events = events_result.get("items", [])

    if not events:
        print("No upcoming events found.")
        return

    # Prints the start and name of the next 10 events
    for event in events:
        start = event["start"].get("dateTime", event["start"].get("date"))
        print(start, event["summary"])

    return events


def insert_event_example(service):
    """
    Guide: https://developers.google.com/calendar/api/guides/create-events#python
    Reference: https://developers.google.com/calendar/api/v3/reference/events/insert
    """
    now = datetime.datetime.utcnow()
    start_time_str = now.isoformat() + "Z"
    end_time_str = (now + datetime.timedelta(minutes=30)).isoformat() + "Z"

    # All possible properties: https://developers.google.com/calendar/api/v3/reference/events#resource
    eventBody = {
        "summary": "TEST Summary",
        "description": "TEST Description",
        "start": {
            "dateTime": start_time_str
        },
        "end": {
            "dateTime": end_time_str
        },
        "attendees": [{
            "email": RESOURCE_CALENDAR_ID
        }]
    }

    event = service.events().insert(calendarId="primary",
                                    body=eventBody).execute()

    if not event:
        print("Could not insert event.")
        return

    # Prints the start and name of the event
    start = event["start"].get("dateTime", event["start"].get("date"))
    print(start, event["summary"])

    return event


def patch_event_example(service, event_id):
    """
    Reference: https://developers.google.com/calendar/api/v3/reference/events/patch
    """
    now = datetime.datetime.utcnow()
    end_time_str = now.isoformat() + "Z"

    # set any event values to be changed
    eventBodyChanges = {
        # "summary": "PATCHED TEST Summary",
        "end": {
            "dateTime": end_time_str
        },
    }

    event = service.events().patch(calendarId=RESOURCE_CALENDAR_ID,
                                   eventId=event_id,
                                   body=eventBodyChanges).execute()

    if not event:
        print("Could not patch event.")
        return

    # Prints the start and name of the event
    start = event["start"].get("dateTime", event["start"].get("date"))
    print(start, event["summary"])


def auth():
    creds = None
    if os.path.exists(TOKEN_FILE):
        creds = Credentials.from_authorized_user_file(TOKEN_FILE, SCOPES)

        if not creds.valid:
            if creds.expired and creds.refresh_token:
                creds.refresh(Request())

                with open(TOKEN_FILE, "w") as token:
                    token.write(creds.to_json())

    if not creds:
        raise Exception(
            "Run oauth2flow.py first to generate a valid token.json")

    return creds


def main():
    creds = auth()

    try:
        service = build("calendar", "v3", credentials=creds)

        (current, next) = get_current_and_next_event(service)

        print()
        # events = list_events_example(service)
        # #
        # patch_event_example(service, event.get("id"))

    except HttpError as error:
        print("An error occurred", error)


if __name__ == "__main__":
    main()