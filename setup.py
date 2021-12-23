import shutil
import os
import json
import subprocess
import sys

print("Setting up workspace")

print("Installing dependencies")
dependencies = [
	"requests", # for url data
	"pyunpack", # for unpacking 7z archives
	"patool", # pyunpack requirement
]
#for package in dependencies:
#	subprocess.check_call([sys.executable, "-m", "pip", "install", package])

print("Coping freetype dll")
shutil.copyfile("./crystal-sphinx/temportal-engine/editor/bin/freetype/x86_64/freetype.dll", "./target/debug/freetype.dll")

if not os.path.isdir('tools'):
	os.mkdir(os.path.join(os.getcwd(), 'tools'))
os.chdir('tools')

def download_tracy():
	import requests
	import patool
	from pyunpack import Archive
	if not os.path.isdir('Tracy'):
		os.mkdir(os.path.join(os.getcwd(), 'Tracy'))
	os.chdir('Tracy')
	latest_tracy_release = "https://api.github.com/repos/wolfpld/tracy/releases/latest"
	response = json.loads(requests.get(latest_tracy_release).text)
	tracy_url = f"{response['tag_name']}"
	for asset in response['assets']:
		destination_name = asset['name']
		if asset['name'].endswith('.7z'):
			destination_name = 'Tracy.7z'
		elif asset['name'] == 'tracy.pdf':
			pass
		else:
			continue
		print(f"Downloading {destination_name} from {asset['browser_download_url']}")
		request = requests.get(asset['browser_download_url'], allow_redirects=True)
		with open(destination_name, 'wb') as f:
			f.write(request.content)
	print('Extracting Tracy executable')
	Archive('Tracy.7z').extractall('.')
	os.chdir('../')
download_tracy()

os.chdir('../')

print("Setup complete")
