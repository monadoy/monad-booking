<script lang="ts">
	import { onMount } from "svelte"
	import { timeZonesNames } from "@vvo/tzdb"
	import { Config, defaultConfig } from "./configTypes"

	const validateEmail = email => {
		return String(email)
			.toLowerCase()
			.match(
				/^(([^<>()[\]\\.,;:\s@"]+(\.[^<>()[\]\\.,;:\s@"]+)*)|(".+"))@((\[[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\])|(([a-zA-Z\-0-9]+\.)+[a-zA-Z]{2,}))$/
			)
	}

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

	const validateConfig = () => {
		if (config.wifi.ssid.length == 0) {
			return "Empty WIFI SSID"
		} else if (!validateEmail(config.gcalsettings.calendarid)) {
			return "Invalid calendar ID"
		} else if (config.gcalsettings.token === null) {
			return "Empty or malformed token.json"
		}
		return ""
	}

	const submit = () => {
		const errorMsg = validateConfig()
		if (errorMsg) {
			message = { isError: true, content: `Didn't submit, reason: ${errorMsg}` }
			return
		}

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
					<label for={`awakeday${day}`}
						>{day}
						<input
							type="checkbox"
							id={`awakeday${day}`}
							bind:checked={config.awake.weekdays[day]}
						/></label
					>
				{/each}
			</div>

			<label for="calendarid">Calendar ID</label>
			<input id="calendarid" type="email" bind:value={config.gcalsettings.calendarid} />
			<label for="tokenjson">Token.json</label>
			<input id="tokenjsonfile" type="file" bind:files />
			<div />
			<textarea
				rows="10"
				id="tokenjson"
				placeholder="< choose file OR paste file contents here >"
				bind:value={tokenString}
			/>
			<button id="submit" type="submit">Submit</button>
		</form>
		{#if message.content}
			<div class={message.isError ? "error" : "ok"}>{message.content}</div>
		{/if}
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
	}

	label,
	h4 {
		display: inline-block;
		margin: 0.4rem 0 0.2rem 0;
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
		grid-template-rows: repeat(8, minmax(1.8rem, auto));
		grid-template-columns: minmax(0, auto);
	}

	form > #submit {
		margin: 8px auto;
		font-size: 1.2rem;
		padding: 8px;
	}

	.ok {
		padding: 8px;
		border: 1px solid blue;
	}
	.error {
		padding: 8px;
		border: 1px solid red;
	}

	input[type="file"] {
		margin: 0 0 0.4rem 0;
	}

	input.time {
		width: 50px;
		text-align: center;
	}

	.multi-input {
		display: flex;
		align-items: stretch;
		flex-flow: row wrap;
		gap: 8px;
	}

	.multi-input > input {
		display: flex;

		margin-right: 8px;
	}

	input[type="checkbox"] {
		width: 1.2rem;
		height: 1.2rem;
		margin: 0 8px 0 0;
		transform: translatey(+4px);
	}

	@media only screen and (min-width: 768px) {
		main {
			padding: 1em;
		}

		form {
			grid-template-columns: auto 1fr;
			grid-gap: 0.8rem;
		}

		form > #submit {
			grid-column: span 2;
		}

		label {
			margin: auto 0;
		}

		input[type="file"] {
			margin: auto 0;
		}
	}
</style>
