//@ts-check
import { createServer } from 'http';
import { join } from 'path';
import { createReadStream } from 'fs';
import { stat } from 'fs/promises';

const PORT = 3000;

const base_directory = 'npm-registry-proxy-offline-cache'

/*
Hosts a proxy on http://localhost:3000 that
serves/replays the packages that were recorded in tool_record.mjs

This proxy pretends to be a registry, we set the pnpm registry to it to make pnpm use it.
*/
const server = createServer(async (req, res) => {
    if (!req.url) {
        res.writeHead(400)
        return res.end('URL missing')
    }

    let filePath = decodeURIComponent(join(base_directory, req.url.endsWith(".tgz") ? req.url : join(req.url, 'index.json')))
    console.log(req.url, filePath);


    try {
        const stats = await stat(filePath)
        res.writeHead(200, {
            'Content-Type': req.url.endsWith(".tgz") ? 'application/octet-stream' : 'text/json',
            'Content-Length': stats.size,
        });

        // Create a readable stream from the file and pipe it to the response
        const readStream = createReadStream(filePath);
        readStream.pipe(res);
        readStream.on('end', () => {
            res.end()
        })

        // Handle stream errors
        readStream.on('error', (streamErr) => {
            console.error(req.url, { streamErr });
            res.writeHead(500, { 'Content-Type': 'text/plain' });
            res.end('Internal Server Error');
        });
    } catch (err) {
        console.error(`Error: ${err.message}`, req.url);
        res.writeHead(404);
        res.end('File not found in cache');
    }
})

// Start the server
server.listen(PORT, () => {
    console.log(`Proxy server is running on http://localhost:${PORT}`);
});