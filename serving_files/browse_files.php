<?php
// Allow CLI invocation with key=value args (e.g., php browse_files.php 'dir=foo/bar')
if (php_sapi_name() === 'cli' && isset($argv) && is_array($argv)) {
    for ($i = 1; $i < count($argv); $i++) {
        if (strpos($argv[$i], '=') !== false) {
            parse_str($argv[$i], $kv);
            foreach ($kv as $k => $v) {
                $_GET[$k] = $v;
            }
        }
    }
}
$currentDir = '.';

if (isset($_GET['dir'])) {
    $requestedDir = $_GET['dir'];
    if (strpos($requestedDir, '..') === false) {
        $currentDir = $requestedDir;
    } else {
        // Prevent directory traversal
        $currentDir = '.';
    }
}

echo "<!DOCTYPE html>";
echo "<html lang='en'>";
echo "<head>";
echo '<link rel="stylesheet" href="style/web/hack.css">';
echo '<link rel="stylesheet" href="style/main.css">';
echo "<meta charset='UTF-8'>";
echo "<title>Browse Files</title>";
echo "</head>";
echo "<body>";
echo "<h2>Files in " . htmlspecialchars($currentDir) . "</h2>";

if ($currentDir != '.') {
    $parentDir = dirname($currentDir);
    echo "<a href='browse_files.php?dir=" . urlencode($parentDir) . "'>📁 ../</a><br><br>";
}

$items = scandir('./serving_files/' . $currentDir);
$directories = [];
$files = [];

foreach ($items as $item) {
    if ($item == '.' || $item == '..') {
        continue;
    }
    $itemPath = ($currentDir == '.') ? $item : $currentDir . '/' . $item;
    $fullPath = './serving_files/' . $itemPath;
    
    if (is_dir($fullPath)) {
        $directories[] = ['name' => $item, 'path' => $itemPath];
    } else {
        $files[] = ['name' => $item, 'path' => $itemPath];
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
        echo "<li>📁 <a href='browse_files.php?dir=" . urlencode($dir['path']) . "'>" . htmlspecialchars($dir['name']) . "/</a></li>";
    }
}
echo "</ul>";

// If specific file is requested, redirect to full-page code view
if (isset($_GET['file'])) {
    $requestedFile = $_GET['file'];
    if (strpos($requestedFile, '..') === false) {
        $redir = 'code_view.php?file=' . urlencode($requestedFile);
        if (isset($_GET['dir'])) { $redir .= '&dir=' . urlencode($_GET['dir']); }
        header('Location: ' . $redir);
        exit;
    }
}

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
        
        // Only route code files to code view; others should be served normally via raw_file
        $isCode = in_array(strtolower($ext), ['php', 'cpp', 'c', 'hpp', 'h']);
        if ($isCode) {
            echo "<li>$icon <a href='code_view.php?file=" . urlencode($file['path']) . "&dir=" . urlencode($currentDir) . "'>" . htmlspecialchars($file['name']) . "</a></li>";
        } else {
            echo "<li>$icon <a href='?raw_file=" . urlencode($file['path']) . "'>" . htmlspecialchars($file['name']) . "</a></li>";
        }
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