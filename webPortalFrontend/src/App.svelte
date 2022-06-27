<script lang="ts">
	import { onMount } from "svelte"
	import { timeZonesNames } from "@vvo/tzdb"

	const validateEmail = email => {
		return String(email)
			.toLowerCase()
			.match(
				/^(([^<>()[\]\\.,;:\s@"]+(\.[^<>()[\]\\.,;:\s@"]+)*)|(".+"))@((\[[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\])|(([a-zA-Z\-0-9]+\.)+[a-zA-Z]{2,}))$/
			)
	}

	let defaultConfig: Config = {
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
	}

	interface Config {
		name?: string
		timezone?: string
		gcalsettings: {
			calendarid?: string
			token: any
		}
		wifi?: {
			ssid?: string
			password?: string
		}
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

	let config: Config | null = defaultConfig

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
			<label for="calendarid">Calendar ID</label>
			<input id="calendarid" type="text" bind:value={config.gcalsettings.calendarid} />
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

	label {
		display: inline-block;
		margin: 0.4rem 0 0.2rem 0;
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
		margin: auto 0;
	}

	@media only screen and (min-width: 768px) {
		main {
			padding: 1em;
		}

		form {
			grid-template-columns: auto 1fr;
			grid-gap: 0.5rem;
		}

		form > #submit {
			grid-column: span 2;
		}

		label {
			margin: auto 0;
		}
	}
</style>
