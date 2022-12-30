export let defaultConfig: Config = {
	name: "",
	timezone: "Europe/Helsinki",
	calendar_provider: "google",
	gcalsettings: {
		calendarid: "",
		token: undefined,
	},
	mscalsettings: {
		room_email: "",
		token: undefined,
	},
	wifi: {
		ssid: "",
		password: "",
	},
	awake: {
		time: {
			from: "07:00",
			to: "19:00",
		},
		weekdays: {
			mon: true,
			tue: true,
			wed: true,
			thu: true,
			fri: true,
			sat: false,
			sun: false,
		},
	},
	language: "FI",
	autoupdate: false,
	update_channel: "stable",
}

export interface Config {
	name: string
	timezone: string
	gcalsettings: {
		calendarid: string
		token?: {
			scope: string
			client_id: string
			client_secret: string
			refresh_token: string
		}
	}
	calendar_provider: "google" | "microsoft"
	mscalsettings: {
		room_email: string
		token: {
			scope: string
			client_id: string
			refresh_token: string
		}
	}
	wifi: {
		ssid: string
		password: string
	}
	awake: {
		time: {
			from: string
			to: string
		}
		weekdays: {
			mon: boolean
			tue: boolean
			wed: boolean
			thu: boolean
			fri: boolean
			sat: boolean
			sun: boolean
		}
	}
	language: string
	autoupdate: boolean
	update_channel: "stable" | "beta"
}
