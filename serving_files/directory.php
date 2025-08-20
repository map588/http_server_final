<?php
$currentDir = '.';

if (isset($argv[1])) {
    $requestedDir = $argv[1];
    if (strpos($requestedDir, '..') === false) {
        $currentDir = $requestedDir;
    }
}

$fullPath = './serving_files/' . $currentDir;

if (!is_dir($fullPath)) {
    echo "<!DOCTYPE html>";
    echo "<html lang='en'>";
    echo "<head>";
    echo '<link rel="stylesheet" href="style/web/hack.css">';
    echo '<link rel="stylesheet" href="style/main.css">';
    echo "<meta charset='UTF-8'>";
    echo "<title>Error</title>";
    echo "</head>";
    echo "<body>";
    echo "<h2>Error: Not a directory</h2>";
    echo "<p>The requested path is not a directory.</p>";
    echo "<h2><a href='index.php'>Back to Main Page</a></h2>";
    echo "</body>";
    echo "</html>";
    exit;
}

echo "<!DOCTYPE html>";
echo "<html lang='en'>";
echo "<head>";
echo '<link rel="stylesheet" href="style/web/hack.css">';
echo '<link rel="stylesheet" href="style/main.css">';
echo "<meta charset='UTF-8'>";
echo "<title>Directory: " . htmlspecialchars($currentDir) . "</title>";
echo "</head>";
echo "<body>";
echo "<h2>Directory: " . htmlspecialchars($currentDir) . "</h2>";

if ($currentDir != '.') {
    $parentDir = dirname($currentDir);
    echo "<a href='?dir=" . urlencode($parentDir) . "'>📁 ../</a><br><br>";
}

$items = scandir($fullPath);
$directories = [];
$files = [];

foreach ($items as $item) {
    if ($item == '.' || $item == '..') {
        continue;
    }
    $itemPath = $fullPath . '/' . $item;
    $relativePath = ($currentDir == '.') ? $item : $currentDir . '/' . $item;
    
    if (is_dir($itemPath)) {
        $directories[] = ['name' => $item, 'path' => $relativePath];
    } else {
        $files[] = ['name' => $item, 'path' => $relativePath];
    }
}

sort($directories);
sort($files);

echo "<h3>Directories:</h3>";
echo "<ul>";
if (empty($directories)) {
    echo "<li><em>No subdirectories</em></li>";
} else {
    foreach ($directories as $dir) {
        echo "<li>📁 <a href='?dir=" . urlencode($dir['path']) . "'>" . htmlspecialchars($dir['name']) . "/</a></li>";
    }
}
echo "</ul>";

echo "<h3>Files:</h3>";
echo "<ul>";
if (empty($files)) {
    echo "<li><em>No files</em></li>";
} else {
    foreach ($files as $file) {
        $ext = pathinfo($file['name'], PATHINFO_EXTENSION);
        $icon = '📄';
        if (in_array($ext, ['jpg', 'jpeg', 'png', 'gif'])) $icon = '🖼️';
        elseif (in_array($ext, ['cpp', 'c', 'h', 'hpp'])) $icon = '📝';
        elseif (in_array($ext, ['php', 'js', 'css', 'html'])) $icon = '🌐';
        elseif (in_array($ext, ['txt', 'md'])) $icon = '📋';
        
        echo "<li>$icon <a href='" . htmlspecialchars($file['path']) . "'>" . htmlspecialchars($file['name']) . "</a></li>";
    }
}
echo "</ul>";

echo "<hr>";
echo "<h3>Directory Info:</h3>";
echo "<p>Total items: " . (count($directories) + count($files)) . " (" . count($directories) . " directories, " . count($files) . " files)</p>";

echo "<h2>----------------------------</h2>";
echo "<h2><a href='index.php'>Back to Main Page</a></h2>";
echo "</body>";
echo "</html>";