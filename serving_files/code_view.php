<?php
// Allow CLI invocation with key=value args (e.g., php code_view.php 'file=foo/bar.cpp' 'dir=foo')
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

$requestedFile = isset($_GET['file']) ? $_GET['file'] : '';
$currentDir = isset($_GET['dir']) ? $_GET['dir'] : '.';

// Basic validation
if ($requestedFile === '' || strpos($requestedFile, '..') !== false) {
    http_response_code(400);
    echo "<html><body><h3>Bad request</h3><p>Missing or invalid file parameter.</p></body></html>";
    exit;
}

$filePath = './serving_files/' . $requestedFile;
if (!is_file($filePath) || !is_readable($filePath)) {
    http_response_code(404);
    echo "<html><body><h3>Not found</h3><p>Unable to read file: " . htmlspecialchars($requestedFile) . "</p></body></html>";
    exit;
}

$ext = strtolower(pathinfo($requestedFile, PATHINFO_EXTENSION));
$langClass = '';
if ($ext === 'php') $langClass = 'language-php';
elseif ($ext === 'cpp' || $ext === 'cc' || $ext === 'cxx' || $ext === 'c') $langClass = 'language-cpp';
elseif ($ext === 'hpp' || $ext === 'h') $langClass = 'language-cpp';

// Guard: only allow code file types; otherwise redirect back to browse
if ($langClass === '') {
    $backLink = ($currentDir === '.' || $currentDir === '') ? 'browse_files.php' : ('browse_files.php?dir=' . urlencode($currentDir));
    header('Location: ' . $backLink);
    exit;
}

$code = htmlspecialchars(file_get_contents($filePath));

// Build back link
$backLink = ($currentDir === '.' || $currentDir === '')
    ? 'browse_files.php'
    : ('browse_files.php?dir=' . urlencode($currentDir));

echo "<!DOCTYPE html>";
echo "<html lang='en'>";
echo "<head>";
echo "<meta charset='UTF-8'>";
echo "<meta name='viewport' content='width=device-width, initial-scale=1'>";
echo "<title>" . htmlspecialchars($requestedFile) . "</title>";
// Minimal CSS to give full-page code view without overriding highlight styles
echo "<style>
  body { margin: 0; background: #0d1117; color: #c9d1d9; font-family: -apple-system, BlinkMacSystemFont, \"Segoe UI\", Roboto, Helvetica, Arial, sans-serif; }
  header { padding: 12px 16px; background: #161b22; border-bottom: 1px solid #30363d; display: flex; align-items: center; gap: 12px; }
  header a { color: #58a6ff; text-decoration: none; }
  header a:hover { text-decoration: underline; }
  header .path { color: #8b949e; font-size: 14px; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
  pre { margin: 0; padding: 16px; overflow: auto; }
  code { display: block; font-size: 13px; line-height: 1.5; }
</style>";
// Minimal fallback highlight colors in case CDN CSS fails
echo "<style>
  .hljs { color: #c9d1d9; background: #0d1117; }
  .hljs-keyword, .hljs-selector-tag, .hljs-subst { color: #ff7b72; font-weight: 600; }
  .hljs-literal, .hljs-number, .hljs-attr { color: #79c0ff; }
  .hljs-string, .hljs-doctag, .hljs-regexp { color: #a5d6ff; }
  .hljs-title, .hljs-name, .hljs-section { color: #d2a8ff; }
  .hljs-comment, .hljs-quote { color: #8b949e; font-style: italic; }
  .hljs-type, .hljs-class .hljs-title { color: #ffa657; }
  .hljs-function .hljs-title, .hljs-function .hljs-params { color: #7ee787; }
  .hljs-meta, .hljs-meta .hljs-keyword { color: #79c0ff; }
  .hljs-builtin-name, .hljs-attribute { color: #ffa657; }
</style>";
// Highlight.js stylesheet only; scripts are loaded at end of body to ensure execution order
echo '<link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/highlight.js/11.9.0/styles/github-dark.min.css">';
echo "</head>";
echo "<body>";
echo "<header>";
echo "<a href='" . $backLink . "'>&larr; Back</a>";
echo "<span class='path'>" . htmlspecialchars($requestedFile) . "</span>";
echo "</header>";
echo "<pre><code class='hljs " . $langClass . "'>" . $code . "</code></pre>";
// Scripts placed at end to avoid any race with DOM readiness
echo '<script src="https://cdnjs.cloudflare.com/ajax/libs/highlight.js/11.9.0/highlight.min.js"></script>';
echo '<script src="https://cdnjs.cloudflare.com/ajax/libs/highlight.js/11.9.0/languages/cpp.min.js"></script>';
echo '<script src="https://cdnjs.cloudflare.com/ajax/libs/highlight.js/11.9.0/languages/php.min.js" onload="if(window.hljs){hljs.highlightAll();}"></script>';
echo "</body>";
echo "</html>";
?>


