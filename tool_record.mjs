//@ts-check
import { createServer } from 'http';
import { request } from 'https';
import { createHash } from 'crypto';
import { basename, dirname, join } from 'path';
import { mkdirSync, writeFileSync } from 'fs';

const PORT = 3000;

const flatpakManifest = {}
const flatpakManifestIndices = []

/** @type {string[]} */
const packages_which_should_dl_metadata = []

/** @type {Record<string,string[]>} */
const usedVersions = {}

function stripSuffix(str, suffix) {
    if (str.endsWith(suffix)) {
        return str.slice(0, -suffix.length);
    }
    return str; // Return the original string if it doesn't end with the suffix
}

/* Hosts a proxy to npmjs on http://localhost:3000
that records what packages are accessed
and saves them in the manifest generated/proxy-registry-cache-manifest.json
and the package indices to generated/proxy-registry-cache-indices

This proxy pretends to be a registry, we set the pnpm registry to it to make pnpm use it.
*/
const server = createServer((req, res) => {
    if (!req.url) {
        res.writeHead(400)
        return res.end('URL missing')
    }
    // console.debug(`Request start URL: ${req.method} ${req.url}`);

    const proxyRequest = request({
        hostname: 'registry.npmjs.org',
        port: 443,
        path: req.url,
        method: req.method,
    }, (proxyResponse) => {
        const { 'content-encoding': contentEncoding, 'content-length': contentLength, ...filteredHeaders } = proxyResponse.headers;
        const isJson = proxyResponse.headers['content-type'] && proxyResponse.headers['content-type'].includes('application/json');
        res.writeHead(proxyResponse.statusCode || 500, isJson ? filteredHeaders : proxyResponse.headers);
        // Check if the response is JSON
        // console.log({ isJson });
        const hash = createHash('sha512');
        if (isJson) {
            let data = '';
            proxyResponse.on('data', (chunk) => {
                data += chunk;
                hash.update(chunk)
            });
            proxyResponse.on('end', () => {
                // console.log("end", data.length)
                data = data.replace(/https:\/\/registry\.npmjs\.org/g, 'http://localhost:3000');

                if (req.url) {
                    if (!req.url.endsWith(".tgz")) {
                        const filename = join(req.url, 'index.json')
                        const dest = join(
                            'generated/proxy-registry-cache-indices/',
                            dirname(filename))
                        mkdirSync(dest, { recursive: true })
                        const the_path = join(
                            dest,
                            basename(filename)
                        )

                        let parsed_data = JSON.parse(data);

                        // delete some fields that we don't need
                        delete parsed_data["keywords"]
                        delete parsed_data["repository"]
                        delete parsed_data["contributors"]
                        delete parsed_data["author"]
                        delete parsed_data["bugs"]
                        delete parsed_data["readme"]
                        delete parsed_data["readmeFilename"]
                        delete parsed_data["maintainers"]
                        delete parsed_data["homepage"]
                        delete parsed_data["description"]

                        for (const key in parsed_data["versions"]) {
                            if (Object.prototype.hasOwnProperty.call(parsed_data["versions"], key)) {
                                const element = parsed_data["versions"][key];
                                delete element["keywords"]
                                delete element["repository"]
                                delete element["contributors"]
                                delete element["maintainers"]
                                delete element["author"]
                                delete element["homepage"]
                                delete element["_npmUser"]
                                delete element["_npmOperationalInternal"]
                                delete element["description"]
                            }
                        }

                        // data._rev - could be some hash that we need to adjust because we modified the content?

                        writeFileSync(
                            the_path,
                            // make it take multiple lines, otherwise git diff would not be more efficient than with inlining into builder manifest
                            JSON.stringify(parsed_data, null, 1),
                            'utf-8')
                        flatpakManifestIndices.push({
                            type: "file",
                            path: the_path,
                            dest: join('npm-registry-proxy-offline-cache', req.url),
                            "dest-filename": basename(the_path)
                        })
                    }
                } else {
                    console.log("req.url is undefined");
                }

                res.end(data);
            });
        } else {
            proxyResponse.on('data', (chunk) => {
                hash.update(chunk)
            });
            proxyResponse.pipe(res, { end: true });
        }
        proxyResponse.on('end', () => {
            const sha512 = hash.digest('hex')
            // console.log(`Request end URL: ${req.method} ${req.url}`, { isJson }, sha512);
            if (!req.url) {
                throw new Error("no req url");
            }

            if (req.url.endsWith(".tgz")) {
                packages_which_should_dl_metadata.push(
                    stripSuffix(stripSuffix(req.url, basename(req.url)), '/-/')
                )
            } else {
                return /* because we download index.json files into generated folder now
                    why? because they are not static and might change between generating and building
                    why not type:inline?
                     - githubs file limit is 100mb, the filesize with inline was 207mb
                     - in order to reduce git diff sizes. (single line 3mb json objects might not be effiecient to diff for git?)
                */
            }

            const destPath = join('npm-registry-proxy-offline-cache', req.url.endsWith(".tgz") ? req.url : join(req.url, 'index.json'))
            flatpakManifest[req.url] = {
                type: 'file',
                url: `https://registry.npmjs.org${req.url}`,
                sha512,
                dest: dirname(destPath),
                "dest-filename": basename(destPath),
            }

            let [name, version] = req.url.split("/-/")
            name = name.substring(1) // remove `/` in beginning
            const sub_package_name = name.split('/').slice(-1)[0]
            version = version.replace(`${sub_package_name}-`, '')
            version = version.substring(0, version.lastIndexOf(".tgz"))

            if (usedVersions[name]) {
                usedVersions[name].push(version)
            } else {
                usedVersions[name] = [version]
            }
        })
    });

    // Handle errors
    proxyRequest.on('error', (err) => {
        console.error(`Error: ${err.message}`);
        res.writeHead(500);
        res.end('Internal Server Error');
    });

    // Pipe the request data from the client to the target server
    req.pipe(proxyRequest, { end: true });
});

// Start the server
server.listen(PORT, () => {
    console.log(`Proxy server is running on http://localhost:${PORT}`);
});

async function save() {
    console.log("force downloading of index files");

    // force downloading of index files
    const indexUrls = packages_which_should_dl_metadata.map(relativeUrl => `http://localhost:${PORT}${relativeUrl}`)
    // console.log(indexUrls);

    // TODO before downloadin, remove what we already downloaded. 

    await Promise.all(indexUrls.map(async url => {
        let tries = 0
        while (tries <= 3) {
            try {
                return await fetch(url)
            } catch (error) {
                tries++;
                console.log(`request to ${url} failed: ${error}, we are on attempt ${tries} for this file`);
            }
        }
        return Promise.reject(new Error("requests failed too often"))
    }))

    console.log("processing and saving");
    const arrayManifest = Object.keys(flatpakManifest).map(url => flatpakManifest[url])
    arrayManifest.sort((a, b) => a.url.localeCompare(b.url))

    flatpakManifestIndices.sort((a, b) => a.path.localeCompare(b.path))

    writeFileSync('generated/proxy-registry-cache-manifest.json', JSON.stringify([...arrayManifest, ...flatpakManifestIndices], null, 2), 'utf-8')
    writeFileSync('generated/used_versions_strip_info.json', JSON.stringify(usedVersions, null, 2), 'utf-8')
}

process.on('SIGINT', async (ev) => {
    console.log("saving stage");
    await save()
    process.exit(0)
})

// IDEA: based on url convention or package name we could fuess "only-arches"