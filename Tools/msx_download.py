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

def get_url_paths(url, ext='', params={}):
    response = requests.get(url, params=params)
    if response.ok:
        response_text = response.text
    else:
        return response.raise_for_status()
    soup = BeautifulSoup(response_text, 'html.parser')
    parent = [node.get('href') for node in soup.find_all('a') if node.get('href').endswith(ext)]
    return parent

def download(no, url, dir, filename, targetfile):
    response = requests.get(url)
    print(f'{no}. {filename} ({len(response.content)})')
    os.makedirs(dir, exist_ok=True)
    open(targetfile, 'wb').write(response.content)

URL = 'https://download.file-hunter.com/'
directories = ['Games/MSX1/ROM', 'Games/MSX2/ROM', 'Games/MSX2+/ROM', 'Games/MSX1/DSK', 'Games/MSX2/DSK', 'Games/MSX2+/DSK']
ext = 'zip'
with  ThreadPoolExecutor(max_workers=10) as executor:
    for dir in directories:
        url = f'{URL}{dir}'
        print(url)
        urlcache = dir.replace('/','_') + '.lst'
        if os.path.exists(urlcache):
            result = open(urlcache).read().split('\n')
        else:
            result = get_url_paths(url, ext)
            open(urlcache, 'w').write('\n'.join(result))
        for no, file in enumerate(result):
            filename = urllib.parse.unquote(file)
            if 'GoodMSX' in file or not '[' in filename:
                targetfile = f'{dir}/{filename}'
                if not os.path.exists(targetfile) or os.stat(targetfile).st_size < 1024:
                    executor.submit(download, no, f'{url}/{file}', dir, filename, targetfile)
                else:
                    print(f'{no}. {filename} ({os.stat(targetfile).st_size})')
            else:
                print(f'[x]{no}. {filename} ')