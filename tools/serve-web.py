"""Static-only desktop/phone preview with revalidation and immutable asset caching."""
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from functools import partial
from pathlib import Path
import argparse
import re
from urllib.parse import urlsplit

class GameFiles(SimpleHTTPRequestHandler):
    def list_directory(self, path):
        self.send_error(404)
        return None

    def end_headers(self):
        path=urlsplit(self.path).path
        immutable=bool(re.search(r'/assets/[0-9a-f]{64}\.[a-z0-9]+$',path))
        self.send_header('Cache-Control','public, max-age=31536000, immutable' if immutable else 'no-cache')
        self.send_header('X-Content-Type-Options','nosniff')
        super().end_headers()

if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--directory',type=Path,default=Path('dist/web'))
    parser.add_argument('--bind',default='127.0.0.1')
    parser.add_argument('--port',type=int,default=8765)
    args=parser.parse_args()
    root=args.directory.resolve()
    if not (root/'index.html').is_file(): raise SystemExit('Package the game first')
    print(f'Serving game only: http://{args.bind}:{args.port}/index.html',flush=True)
    ThreadingHTTPServer((args.bind,args.port),partial(GameFiles,directory=str(root))).serve_forever()
