export let defaultConfig: Config = {
	name: "",
	timezone: "Europe/Helsinki",
	gcalsettings: {
		calendarid: "",
		token: null,
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
		}
	},
	language: "FI",
	autoUpdate: false,
}

export interface Config {
	name: string
	timezone: string
	gcalsettings: {
		calendarid: string
		token: any
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
	language: string,
	autoUpdate: boolean
}