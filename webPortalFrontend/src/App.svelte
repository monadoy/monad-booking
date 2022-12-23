<script lang="ts">
	import { onMount } from "svelte"
	import { timeZonesNames } from "@vvo/tzdb"
	import { type Config, defaultConfig } from "./configTypes"

	const safeParseInt = (input: string) => {
		const parsed = parseInt(input, 10)
		if (isNaN(parsed)) {
			return 0
		}
		return parsed
	}

	const parseTimeInput = (input: string) => {
		let [hInput, minInput] = input.split(":")
		hInput ??= ""
		minInput ??= ""

		const h = Math.min(Math.max(safeParseInt(hInput), 0), 23)
		const min = Math.min(Math.max(safeParseInt(minInput), 0), 59)
		const str = h.toString().padStart(2, "0") + ":" + min.toString().padStart(2, "0")
		return str
	}

	let message = { isError: false, content: "" }
	const clearMessage = (startsWith: string) => {
		if (message.content.startsWith(startsWith)) message = { isError: false, content: "" }
	}

	const submit = () => {
		fetch("/config", {
			method: "POST",
			headers: {
				"Content-Type": "application/json",
			},
			body: JSON.stringify(config),
		})
			.then(res => {
				if (res.ok) message = { isError: false, content: "[Submit success]" }
				else
					message = {
						isError: true,
						content: `[Submit failure] HTTP status: ${res.status} ${res.statusText}`,
					}
			})
			.catch(err => {
				console.log(err)
				message = { isError: true, content: `[Submit failure] ${err.message}` }
			})
	}

	let config: Config = defaultConfig

	$: console.log(config)

	const updateGoogleToken = (s: string) => {
		if (!s) return
		try {
			config.gcalsettings.token = JSON.parse(s.trim())
			clearMessage("[Malformed google_token.json]")
		} catch (err) {
			config.gcalsettings.token = undefined
			console.log(err)
			message = { isError: true, content: `[Malformed google_token.json] ${err.message}` }
		}
	}

	let googleTokenFiles: FileList
	$: if (googleTokenFiles && googleTokenFiles.length > 0) {
		googleTokenFiles[0].text().then(text => (googleTokenString = text))
	}
	

	let googleTokenString = ""
	$: if (googleTokenString) {
		try {
			config.gcalsettings.token = JSON.parse(googleTokenString.trim())
			clearMessage("[Malformed google_token.json]")
		} catch (err) {
			config.gcalsettings.token = undefined
			console.log(err)
			message = { isError: true, content: `[Malformed google_token.json] ${err.message}` }
		}
	}

	let microsoftTokenFiles: FileList
	$: if (microsoftTokenFiles && microsoftTokenFiles.length > 0) {
		microsoftTokenFiles[0].text().then(text => (microsoftTokenString = text))
	}

	let microsoftTokenString = ""
	$: if (microsoftTokenString) {
		try {
			config.mscalsettings.token = JSON.parse(microsoftTokenString.trim())
			clearMessage("[Malformed microsoft_token.json]")
		} catch (err) {
			config.mscalsettings.token = undefined
			console.log(err)
			message = { isError: true, content: `[Malformed microsoft_token.json] ${err.message}` }
		}
	}

	let configFetchStatus = "Fetching Monad Booking device config..."
	onMount(() => {
		fetch("/config")
			.then(res => res.json())
			.then(json => {
				config = { ...defaultConfig, ...json }
				try {
					if (config.gcalsettings.token) googleTokenString = JSON.stringify(config.gcalsettings.token)
					if (config.mscalsettings.token) googleTokenString = JSON.stringify(config.mscalsettings.token)
				} catch (err) {}

				// Config server sets these to null when they are hidden
				let hiddenElements = []
				if (config.gcalsettings.token === null) hiddenElements.push("google_token.json")
				if (config.mscalsettings.token === null) hiddenElements.push("microsooft_token.json")
				if (config.wifi.password === null) hiddenElements.push("WIFI password")

				let hiddenLabel = ""
				if (hiddenElements.length > 0) {
					hiddenLabel = `\n(Options hidden for security: ${hiddenElements.join(", ")})`
				}

				configFetchStatus = `Monad Booking device config fetch success ${hiddenLabel}`
			})
			.catch(err => {
				config = defaultConfig
				console.log(err)
				configFetchStatus = "Couldn't fetch Monad Booking device config"
			})
	})
</script>

<main>
	<h1>Monad Booking Device Configuration</h1>
	<p class="status">{configFetchStatus}</p>
	{#if config}
		<form class="content" on:submit|preventDefault={submit}>
			<label for="name">Name</label>
			<input id="name" type="text" bind:value={config.name} />
			<label for="language">Language Code</label>
			<input id="language" type="text" bind:value={config.language} />
			<label for="wifissid">WIFI SSID</label>
			<input id="wifissid" type="text" bind:value={config.wifi.ssid} />
			<label for="wifipassword">WIFI Password</label>
			<input id="wifipassword" type="text" bind:value={config.wifi.password} />
			<label for="timezone">IANA Time Zone</label>
			<select id="timezone" bind:value={config.timezone}>
				{#each timeZonesNames as tz}
					<option value={tz}>
						{tz}
					</option>
				{/each}
			</select>
			<label for="autoupdate">Auto Update</label>
			<div class="multi-input">
				<input type="checkbox" id="autoupdate" bind:checked={config.autoupdate} />
				{#if config.autoupdate}
					(Enabled)
				{:else}
					(Disabled)
				{/if}
			</div>
			<label for="update-channel">Update Channel</label>
			<select id="update-channel" bind:value={config.update_channel}>
				<option value="stable">Stable</option>
				<option value="beta">Beta</option>
			</select>
			<h4>Awake Times</h4>
			<div class="multi-input">
				<label for="awaketimefrom">from</label>
				<input
					class="time"
					id="awaketimefrom"
					type="text"
					on:blur={e => {
						config.awake.time.from = parseTimeInput(e.currentTarget.value)
					}}
					bind:value={config.awake.time.from}
				/>
				<label for="awaketimeto">to</label>
				<input
					class="time"
					id="awaketimeto"
					type="text"
					on:blur={e => {
						config.awake.time.to = parseTimeInput(e.currentTarget.value)
					}}
					bind:value={config.awake.time.to}
				/>
			</div>
			<h4>Awake Days</h4>
			<div class="multi-input">
				{#each ["mon", "tue", "wed", "thu", "fri", "sat", "sun"] as day}
					<label for={`awakeday${day}`}>{day.charAt(0).toUpperCase() + day.slice(1)}</label>
					<input
						type="checkbox"
						id={`awakeday${day}`}
						bind:checked={config.awake.weekdays[day]}
					/>
				{/each}
			</div>
			<label for="calendar-provider">Calendar Provider</label>
			<select id="calendar-provider" bind:value={config.calendar_provider}>
				<option value="google">Google</option>
				<option value="microsoft">Microsoft 365</option>
			</select>
			{#if config.calendar_provider == "google"}
				<label class="inner-option" for="google-calendarid">Calendar ID</label>
				<input id="google-calendarid" type="email" bind:value={config.gcalsettings.calendarid} />
				<label class="inner-option" for="google-token">google_token.json</label>
				<div class="multi-input vertical">
					<input id="google-token-file" type="file" bind:files={googleTokenFiles} />
					<textarea
						rows="8"
						id="google-token"
						placeholder="< choose file OR paste file contents here >"
						bind:value={googleTokenString}
					/>
				</div>
			{:else if config.calendar_provider == "microsoft"}
				<label class="inner-option" for="microsoft-room-email">Room Email</label>
				<input id="microsoft-room-email" type="email" bind:value={config.mscalsettings.room_email} />
				<label class="inner-option" for="microsoft-token">microsoft_token.json</label>
				<div class="multi-input vertical">
					<input id="microsoft-token-file" type="file" bind:files={microsoftTokenFiles} />
					<textarea
						rows="8"
						id="microsoft-token"
						placeholder="< choose file OR paste file contents here >"
						bind:value={microsoftTokenString}
					/>
				</div>
			{/if}
			<button id="submit" type="submit">Submit</button>
			{#if message.content}
				<div class={`message ${message.isError ? "error" : "ok"}`}>{message.content}</div>
			{/if}
		</form>
	{:else}
		<h2>Loading Config...</h2>
	{/if}
</main>

<style>
	:root {
		font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Oxygen, Ubuntu,
			Cantarell, "Open Sans", "Helvetica Neue", sans-serif;
	}

	main {
		padding: 0;
		margin: 0 auto;
		max-width: 800px;
	}

	.status {
		white-space: pre-line;
		line-height: 1.5rem;
	}

	label,
	h4 {
		display: inline-block;
		margin: 0.8rem 0 0.2rem 0;
		/* width: 10rem; */
		font-weight: normal;
		font-size: 1rem;
	}

	label.inner-option {
		padding-left: 1rem;
		font-style: italic;
	}

	textarea {
		white-space: pre;
		overflow-wrap: normal;
		overflow-x: scroll;
		-moz-tab-size: 2;
		-o-tab-size: 2;
		tab-size: 2;
	}

	form {
		display: grid;
		grid-template-columns: 1fr;
		grid-template-rows: repeat(auto-fill, auto);
	}

	button#submit {
		font-size: 1.2rem;
		padding: 0.5rem 2rem;
		margin: 8px auto;
	}

	.message {
		padding: 0.8rem;
		text-align: center;
		background-color: white;
		margin: 2px;
		position: sticky;
		bottom: 4px;
	}

	.message.ok {
		background-color: #ddf;
		border: 2px solid blue;
	}

	.message.error {
		background-color: #fdd;
		border: 2px solid red;
	}

	input.time {
		width: 70px;
		text-align: center;
	}

	.multi-input {
		display: flex;
		align-items: center;
		flex-flow: row wrap;
		gap: 8px;
	}

	.multi-input.vertical {
		align-items: stretch;
		flex-flow: column wrap;
	}

	.multi-input > label {
		margin: 0;
	}

	input[type="checkbox"] {
		width: 1.2rem;
		height: 1.2rem;
		margin-right: 0.5rem;
	}

	input {
		height: 1.2rem;
		margin: 2px;
	}

	select {
		height: 1.7rem;
		margin: 2px;
	}

	input[type="file"] {
		height: auto;
	}

	@media only screen and (min-width: 768px) {
		main {
			padding: 1em;
		}

		form {
			grid-template-columns: 10rem 1fr;
			grid-gap: 0.8rem;
		}

		label {
			margin: 0.4rem 0 0 0;
		}

		button#submit,
		.message {
			grid-column: span 2;
		}
	}
</style>
