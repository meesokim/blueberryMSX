import os
from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.webdriver.chrome.options import Options
from selenium.webdriver.chrome.service import Service
from webdriver_manager.chrome import ChromeDriverManager
import requests
import time
import urllib.parse
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.common.exceptions import TimeoutException

def download_image(url, file_path):
    response = requests.get(url, stream=True)
    if response.status_code == 200:
        with open(file_path, 'wb') as f:
            f.write(response.content)
        return True
    return False

def download_base64_image(base64_string, file_path):
    import base64
    # Remove header if present
    if ',' in base64_string:
        base64_string = base64_string.split(',')[1]
    
    # Decode base64 string
    image_data = base64.b64decode(base64_string)
    
    # Save image
    with open(file_path, 'wb') as f:
        f.write(image_data)
    return True
            
def download_machine_images():
    # Setup Chrome options
    chrome_options = Options()
    chrome_options.add_argument('--headless')  # Run in background
    chrome_options.add_argument('--no-sandbox')
    chrome_options.add_argument('--disable-dev-shm-usage')

    # service = Service('./chrome-linux64/chrome', options=chrome_options)
    # service.start()
    # driver = webdriver.Remote(service.service_url)
    # Initialize the browser
    # driver = webdriver.Chrome('chrome-linux64/chrome')
    driver = webdriver.Chrome(service=Service(executable_path=ChromeDriverManager().install()),options=chrome_options)
    # driver = webdriver.Chrome(service=Service(executable_path=r'chrome-linux64/chrome'),options=chrome_options)
    # driver = webdriver.Chrome(options=chrome_options)
    # Read machines file
    with open('machines', 'r') as f:
        machines = f.readlines()

    # Create output directory
    output_dir = "images"
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    # Process each machine
    for machine in machines:
        machine = machine.strip()
        if not machine:
            continue
        print(machine)
        try:
            # Prepare search term and URL
            search_term = f"msx computer real machine photo {machine}"
            encoded_search = urllib.parse.quote(search_term)
            search_url = f"https://www.google.com/search?q={encoded_search}&udm=2"

            # Load the page
            driver.get(search_url)
            time.sleep(2)  # Wait for page load

            # Find first image
            images = driver.find_elements(By.CSS_SELECTOR, 'img')
            if images:
                for image in images:
                    if image is not None:
                        img_url = image.get_attribute('src')
                        if img_url is not None and 'data:image' in img_url:
                            print(img_url)
                            break
                # In the main loop, modify the image download part:
                if img_url and img_url.startswith('data:image'):
                    print(img_url)
                    file_name = f"{machine.replace(' ', '_').replace('/', '_')}.jpg"
                    file_path = os.path.join(output_dir, file_name)
                    print(f"Downloading base64 image for {machine}")
                    if download_base64_image(img_url, file_path):
                        print(f"Successfully downloaded: {file_name}")
                    else:
                        print(f"Failed to download: {file_name}")
                elif img_url and img_url.startswith('http'):
                    # Download image
                    file_name = f"{machine.replace(' ', '_').replace('/', '_')}.jpg"
                    file_path = os.path.join(output_dir, file_name)
                    
                    print(f"Downloading {machine} from {img_url}")
                    if download_image(img_url, file_path):
                        print(f"Successfully downloaded: {file_name}")
                    else:
                        print(f"Failed to download: {file_name}")
            else:
                print('no image')
            time.sleep(1)  # Delay between searches

        except Exception as e:
            import traceback
            print(f"Error processing {machine}: {str(e)}")
            traceback.print_exc()

    driver.quit()

if __name__ == "__main__":
    download_machine_images()