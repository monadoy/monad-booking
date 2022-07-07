<script lang="ts">
	import { onMount } from "svelte"
	import { timeZonesNames } from "@vvo/tzdb"
	import { Config, defaultConfig } from "./configTypes"

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

	const updateToken = (s: string) => {
		if (!s) return
		try {
			config.gcalsettings.token = JSON.parse(s.trim())
			clearMessage("[Malformed token.json]")
		} catch (err) {
			config.gcalsettings.token = null
			console.log(err)
			message = { isError: true, content: `[Malformed token.json] ${err.message}` }
		}
	}

	let files: FileList
	$: if (files && files.length > 0) {
		files[0].text().then(text => (tokenString = text))
	}

	let tokenString = ""
	$: updateToken(tokenString)

	let configFetchStatus = "Fetching M5Paper config..."
	onMount(() => {
		fetch("/config")
			.then(res => res.json())
			.then(json => {
				config = { ...defaultConfig, ...json }
				try {
					if (config.gcalsettings.token !== null)
						tokenString = JSON.stringify(config.gcalsettings.token)
				} catch (err) {}
				configFetchStatus = "M5Paper config fetch success"
			})
			.catch(err => {
				config = defaultConfig
				console.log(err)
				configFetchStatus = "Couldn't fetch M5Paper config"
			})
	})
</script>

<main>
	<h1>M5Paper Configuration</h1>
	<p>{configFetchStatus}</p>
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
			<select id="timezone" value={config.timezone}>
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
			<label for="calendarid">Calendar ID</label>
			<input id="calendarid" type="email" bind:value={config.gcalsettings.calendarid} />
			<label for="tokenjson">Token.json</label>
			<div class="multi-input vertical">
				<input id="tokenjsonfile" type="file" bind:files />
				<textarea
					rows="10"
					id="tokenjson"
					placeholder="< choose file OR paste file contents here >"
					bind:value={tokenString}
				/>
			</div>
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

	label,
	h4 {
		display: inline-block;
		margin: 0.8rem 0 0.2rem 0;
		font-weight: normal;
		font-size: 1rem;
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
			grid-template-columns: auto 1fr;
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
