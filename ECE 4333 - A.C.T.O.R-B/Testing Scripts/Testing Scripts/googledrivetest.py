from google.oauth2 import service_account
from googleapiclient.discovery import build
import os

def test_drive_connection(credentials_path, folder_id):
    try:
        # Setup credentials with broader scope
        credentials = service_account.Credentials.from_service_account_file(
            credentials_path,
            scopes=['https://www.googleapis.com/auth/drive']  # Changed from drive.file to drive
        )
        
        # Create Drive API service
        service = build('drive', 'v3', credentials=credentials)
        
        # Try to get folder info
        folder = service.files().get(fileId=folder_id, fields='name').execute()
        
        print("Connection successful!")
        print(f"Connected to folder: {folder['name']}")
        return True
        
    except Exception as e:
        print(f"Connection failed: {str(e)}")
        return False

if __name__ == "__main__":
    CREDENTIALS_PATH = os.path.expanduser('~/.config/broadcast-system/service-account.json')
    FOLDER_ID = '101EdO7xAkKAYm8U5FGvmt8erNLFBoAxF'
    
    test_drive_connection(CREDENTIALS_PATH, FOLDER_ID)