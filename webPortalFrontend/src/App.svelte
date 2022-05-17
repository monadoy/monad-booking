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

	let submitOk: boolean | null = null
	const submit = async () => {
		const response = await fetch("/config", {
			method: "POST",
			body: JSON.stringify(config),
		})

		submitOk = response.ok
	}

	let config: Config | null = defaultConfig
	$: console.log(config)

	let files: FileList
	$: if (files && files.length > 0) {
		files[0].text().then(text => (config.gcalsettings.token = JSON.parse(text)))
	}

	let configFetchStatus = "Fetching M5Paper config..."
	onMount(() => {
		fetch("/config")
			.then(res => res.json())
			.then(json => {
				config = { ...defaultConfig, ...json }
				configFetchStatus = "M5Paper config fetch success"
			})
			.catch(err => {
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
				<input id="tokenjson" type="file" bind:files />
			</div>
			<button type="submit">Submit</button>
		</form>
		{#if submitOk !== null}
			{#if submitOk === true}
				<div class="submit-status-ok">Submit success</div>
			{:else}
				<div class="submit-status-fail">Submit failure</div>
			{/if}
		{/if}

		<p>Token.json contents:</p>
		{#if config.gcalsettings.token}
			<pre><code>{JSON.stringify(config.gcalsettings.token, null, 2)}</code></pre>
		{:else}
			<p>empty</p>
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

	input {
		display: inline-block;
		flex-grow: 1;
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

	.submit-status-ok {
		padding: 8px;
		border: 1px solid blue;
	}
	.submit-status-fail {
		padding: 8px;
		border: 1px solid red;
	}
</style>
