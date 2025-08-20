<?php
$currentDir       = 'Executables';
$body_text        = '';

// Process Form
if (isset($_GET['file']) && isset($_GET['arguments'])) {
    $shouldOutputHTML = false;
    $selectedFile     = $_GET['file'];
    $arguments        = $_GET['arguments'];
    $getRequest       = '/command' . $selectedFile . ' ' . $arguments;

    echo $getRequest;
    exit;
}

// Handle output
$body_text =nl2br(htmlspecialchars($argv[1] ?? ''));
$selectedFile = $_GET['file'] ?? '';
$items     = scandir($currentDir);
$listItems = '';

foreach ($items as $item) {
    if ($item === '.' || $item === '..') {
        continue;
    }
    if (file_exists($currentDir . '/' . $item)) {
        $itemName = pathinfo($item, PATHINFO_FILENAME);
        $selectedClass = ($item === $selectedFile) ? ' class="selected"' : '';
        $listItems .= '<li><a href="#" onclick="selectFile(\'' . $item . '\')"' . $selectedClass . '>' . $itemName . "</a></li>\n";
    }
}

// Return HTML
echo <<<BODY
<!DOCTYPE html>
<html lang='en'>
<head>
<link rel="stylesheet" href="style/web/hack.css">
<link rel="stylesheet" href="style/main.css">
<meta charset='UTF-8'>
<title>Choose a program:</title>
</head>
<body>
<h2>Programs</h2>
<ul>
$listItems
</ul>

<form id='fileForm' method='get'>
<input type='hidden' id='selectedFile' name='file'>
<label for='arguments'>Arguments:</label><br>
<input type='text' id='arguments' name='arguments'><br><br>
<input type='submit' value='Submit'>
</form>

<script>
const selectFile = (file) => {
     document.getElementById('selectedFile').value = file;
};
</script>

<h2>Program Output</h2>
<pre>$body_text</pre>

<div class="section-divider"></div>

<h2>Browse Files</h2>
<a href='browse_files.php'>Browse Files</a>

</body>
</html>
BODY;
