import { mount } from 'svelte';
import './lib/m3/tokens.css';
import App from './App.svelte';
import { device } from './lib/status.svelte';

device.start();
mount(App, { target: document.getElementById('app')! });
