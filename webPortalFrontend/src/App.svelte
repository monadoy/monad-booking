<script lang="ts">
	import { onMount } from "svelte"

	let defaultConfig: Config = {
		name: undefined,
		timezone: undefined,
		gcalsettings: {
			calendarid: undefined,
			token: undefined,
		},
		wifi: {
			ssid: undefined,
			password: undefined,
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

	const submit = () => {
		fetch("/config", {
			method: "POST",
			headers: {
				"Content-Type": "application/json",
			},
			body: JSON.stringify(config),
		})
			.then(_ => (message = { isError: false, content: "Submit success" }))
			.catch(err => {
				console.log(err)
				message = { isError: true, content: "Submit failure" }
			})
	}

	let config: Config | null = defaultConfig
	$: console.log(config)

	const updateToken = (s: string) => {
		if (!s) return
		try {
			config.gcalsettings.token = JSON.parse(s.trim())
			message = { isError: false, content: "" }
		} catch (err) {
			console.log(err)
			message = { isError: true, content: "Token.json is malformed" }
		}
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
			<div>
				<label for="name">Name</label>
				<input id="name" type="text" bind:value={config.name} />
			</div>
			<div>
				<label for="wifissid">WIFI SSID</label>
				<input id="wifissid" type="text" bind:value={config.wifi.ssid} />
			</div>
			<div>
				<label for="wifipassword">WIFI Password</label>
				<input id="wifipassword" type="text" bind:value={config.wifi.password} />
			</div>
			<div>
				<label for="timezone">IANA Time Zone</label>
				<input id="timezone" type="text" bind:value={config.timezone} />
			</div>
			<div>
				<label for="calendarid">Calendar ID</label>
				<input id="calendarid" type="text" bind:value={config.gcalsettings.calendarid} />
			</div>
			<div>
				<label for="tokenjson">Token.json</label>
				<textarea rows="10" id="tokenjson" bind:value={tokenString} />
			</div>
			<button type="submit">Submit</button>
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
		padding: 1em;
		margin: 0 auto;
	}

	label {
		display: inline-block;
		width: 150px;
	}

	input,
	textarea {
		display: inline-block;
		flex-grow: 1;
	}

	textarea {
		white-space: pre;
		overflow-wrap: normal;
		overflow-x: scroll;
	}

	form > div {
		margin: 8px;
		display: flex;
		width: 100%;
	}

	form > button {
		margin: 8px auto;
		display: flex;
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
</style>
