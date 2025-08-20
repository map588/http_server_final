<?php
$currentDir = '.';
$shouldOutputHTML = true;

if (isset($_GET['dir'])) {
    $requestedDir = $_GET['dir'];
    if (strpos($requestedDir, '..') === false) {
        $currentDir = $requestedDir;
    } else {
        // Prevent directory traversal
        $currentDir = '.';
    }
}

if (isset($_GET['file'])) {
    $shouldOutputHTML = false;
    $filePath = './serving_files/' . $currentDir . '/' . basename($_GET['file']);
    serveFile($filePath);
}

function serveFile($path) {
    if (!file_exists($path) || is_dir($path)) {
        header("HTTP/1.1 404 Not Found");
        echo "404 Not Found";
        exit;
    }
    header("Content-Disposition: attachment; filename=\"" . basename($path) . "\"");
    readfile($path);
    exit;
}

if ($shouldOutputHTML) {
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
        echo "<a href='?dir=" . urlencode($parentDir) . "'>../</a><br>";
    }

    $items = scandir('./serving_files/' . $currentDir);
    echo "<ul>";
    foreach ($items as $item) {
        if ($item == '.' || $item == '..') {
            continue;
        }
        $itemPath = $currentDir . '/' . $item;
        if (is_dir('./serving_files/' . $itemPath)) {
            echo "<li><a href='?dir=" . urlencode($itemPath) . "'>$item/</a></li>";
        } else {
            echo "<li><a href='?dir=" . urlencode($currentDir) . "&file=" . urlencode($item) . "'>$item</a></li>";
        }
    }
    echo "</ul>";

    echo "<h2>----------------------------</h2>";
    echo "<h2><a href='index.php'>Back to Main Page\n</a></h2>";
    echo "</body>";
    echo "</html>";
}