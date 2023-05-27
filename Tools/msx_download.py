import requests
import urllib
import os
from bs4 import BeautifulSoup
import ssl
import time
from concurrent.futures import ThreadPoolExecutor

if 'http_proxy' in os.environ:
    http_proxy = os.environ['http_proxy']
    proxy = urllib.request.ProxyHandler({'http': http_proxy, 'https': http_proxy})
    # construct a new opener using your proxy settings
    opener = urllib.request.build_opener(proxy)
    # install the openen on the module-level
    urllib.request.install_opener(opener)

def get_url_paths(url, exts='', params={}):
    print(url)
    response = requests.get(url, params=params)
    if response.ok:
        response_text = response.text
    else:
        return response.raise_for_status()
    soup = BeautifulSoup(response_text, 'html.parser')
    parent = []
    for ext in exts:
        parent.extend([node.get('href') for node in soup.find_all('a') if node.get('href').endswith(ext)])
    print('\n'.join(parent))
    if url[-1] ==  '/':
        dirs = [node.get('href') for node in soup.find_all('a') if node.get('href').endswith('/')] 
        for dir in dirs:
            if '..' in dir or 'http' in dir:
                continue
            parent.extend([dir + file for file in get_url_paths(url + dir, exts)])
    return parent

def download(no, url, dir, filename, targetfile):
    response = requests.get(url)
    print(f'{no}. {filename} ({len(response.content)})')
    os.makedirs(dir, exist_ok=True)
    open(targetfile, 'wb').write(response.content)

URL = 'https://download.file-hunter.com/'
directories = ['Games/MSX1/ROM', 'Games/MSX2/ROM', 'Games/MSX2+/ROM', 'Games/MSX1/DSK', 'Games/MSX2/DSK', 'Games/MSX2+/DSK', 'Games/Korean/']
ext = ['zip','rom']
with  ThreadPoolExecutor(max_workers=10) as executor:
    for dir in directories:
        url = f'{URL}{dir}'
        urlcache = dir.replace('/','_') + '.lst'
        if os.path.exists(urlcache):
            result = open(urlcache).read().split('\n')
        else:
            result = get_url_paths(url, ext)
            open(urlcache, 'w').write('\n'.join(result))
        for no, file in enumerate(result):
            filename = urllib.parse.unquote(file)
            if 'GoodMSX' in file or not '[' in filename:
                targetfile = f'{dir}/{filename}'.replace('//','/')
                if '/' in filename:
                    mdir = os.path.dirname(targetfile)
                    filename = targetfile.replace(mdir,'')[1:]
                else:
                    mdir = dir
                if not os.path.exists(targetfile) or os.stat(targetfile).st_size < 1024:
                    executor.submit(download, no, f'{url}/{file}', mdir, filename, targetfile)
                else:
                    print(f'{no}. {filename} ({os.stat(targetfile).st_size})')
