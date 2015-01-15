<?php
$cmd=($_GET['cmd']);
$val=($_GET['val']);

if (file_exists("/tmp/tree.set"))
{
	$settings = file_get_contents("/tmp/tree.set");
	// Mode, Brightness, Speed
	$values = explode(",", $settings);
}
else
{
	$values[0] = "0";
	$values[1] = "1";
	$values[2] = "1";
}

if (!empty($cmd))
{
	$sockf = fsockopen("unix:///tmp/treelights.sock", 0, $errno, $errstr);
	if ($sockf)
	{
		switch ($cmd)
		{
			case 'on':
						$command = $cmd;
						break;
			case 'off':
						$command = $cmd;
						break;
			case 'speed':
						$values[2] = 101 - ($val*10);
						$command = $cmd . " " . $values[2];
						break;
			case 'brightness':
						$values[1] = 11 - $val;
						$command = $cmd . " " . $values[1];
						break;
			case 'program':
						$values[0] = $val;
						$command = $cmd . " " . $values[0];
						break;
		}

		fwrite($sockf, $command);
		fclose($sockf);
		
		$settings = $values[0] . "," . $values[1] . "," . $values[2];
		file_put_contents("/tmp/tree.set", $settings);
	}
}
?>

<html>
<head>
<title>Christmas Tree Lights</title>
</head>
<body>
<p style="text-align: center;">
<select onchange="setProgram(value)">
<?php
	for($i=0; $i<6; $i++)
	{
		echo '<option value="' . $i . '"';
		if ($values[0] == $i) echo ' selected';
		echo '>Mode ' . $i . '</option>';
	}
?>
</select>

<p style="text-align: center;"><label for="brightness">Brightness</label>
<input type="range" min="1" max="10" value="<?php echo (11 - $values[1]);  ?>" id="brightness" step=1 onchange="setBrightness(value)" oninput="displayBrightness(value)">
<output for="brightness" id="bLevel"><?php echo (11 - $values[1]);  ?></output>

<p style="text-align: center;"><label for="speed">Speed</label>
<input type="range" min="1" max="10" value="<?php echo ((101 - $values[2])/10); ?>" id="speed" step=1 onchange="setSpeed(value)" oninput="displaySpeed(value)">
<output for="speed" id="sLevel"><?php echo ((101 - $values[2])/10); ?></output>

<p style="text-align: center;"><button style="margin: 10px;" onclick="onButton()">On</button><button style="margin: 10px;" onclick="offButton()">Off</button>

</body>
<script>
function displayBrightness(brightness) {
	document.querySelector('#bLevel').value = brightness;
}

function displaySpeed(speed) {
	document.querySelector('#sLevel').value = speed;
}

function setBrightness(brightness) {
	url = 'index.php?cmd=brightness&val=';
	location.href = url.concat(brightness);
}

function setSpeed(speed) {
	url = 'index.php?cmd=speed&val=';
	location.href = url.concat(speed);
}

function setProgram(program) {
	url = 'index.php?cmd=program&val=';
	location.href = url.concat(program);
}

function offButton() {
	location.href = 'index.php?cmd=off';
}

function onButton() {
	location.href = 'index.php?cmd=on';
}
</script>
</html>