<H1>ESP data logger</H1>  
<H3>
A tool for logging temprature and humiditiy data from several sensors. <BR />
values are recorded in a txt file in CSV format and accessable from the Web Server running on the ESP<BR /></H3>
<ul>
	<li>current sensor config - ds18b20 , DHT11</li>
	<li>Web Server activated on esp32.local on a self hosted WiFi network, ssid: data logger, pass: 12345678, only when pin d23 pulled HIGH. </li></li>
  <li>file at esp.local/data.txt</li>
</ul>
future upgrades: <br><BR />

<ul>
	<li>web based configurability, incorperating OTA.</li>
	<li>support for all types of sensors and readings</li>
</ul>
