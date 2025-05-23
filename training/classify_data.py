"""
Script to classify images into folders based on whether they contain people or not.

How to Use:
1. Place this script in your project directory.
2. Prepare a folder containing the images you want to classify.
3. Install the required Python packages if not already installed:
   - Run: pip install ultralytics opencv-python
4. Run the script with the input folder as an argument:
   - python classify_data.py <input_folder>
5. After execution, the images will be moved to:
   - 'sorted_images/with_people' (if a person is detected)
   - 'sorted_images/without_people' (if no person is detected)
"""

import os
import sys
import cv2
from ultralytics import YOLO
from concurrent.futures import ThreadPoolExecutor

# Check if input folder is provided
if len(sys.argv) < 2:
    print("Usage: python classify_data.py <input_folder>")
    sys.exit(1)

# Get input folder from command-line arguments
input_dir = sys.argv[1]

# Validate input folder
if not os.path.exists(input_dir):
    print(f"Error: The folder '{input_dir}' does not exist.")
    sys.exit(1)

# Load YOLO model (pre-trained on COCO dataset)
model = YOLO("yolov8n.pt")  # Uses the smallest YOLO model for speed

# Define output directories
output_dir_people = "sorted_images/with_people"
output_dir_no_people = "sorted_images/without_people"

# Create output directories if they don't exist
os.makedirs(output_dir_people, exist_ok=True)
os.makedirs(output_dir_no_people, exist_ok=True)

def process_image(image_name):
    """Process a single image to classify it."""
    image_path = os.path.join(input_dir, image_name)
    
    if not image_path.lower().endswith(('.png', '.jpg', '.jpeg')): 
        return  # Skip non-image files

    # Load image
    img = cv2.imread(image_path)

    # Run YOLO object detection
    results = model(img)

    # Check if a person is detected
    has_person = any('person' in result.names[int(box.cls)] for result in results for box in result.boxes)

    # Move image to respective folder
    target_folder = output_dir_people if has_person else output_dir_no_people
    os.rename(image_path, os.path.join(target_folder, image_name))

    print(f"Processed {image_name}: {'Person detected' if has_person else 'No person detected'}")

# Use ThreadPoolExecutor to process images concurrently
with ThreadPoolExecutor() as executor:
    executor.map(process_image, os.listdir(input_dir))

print("Sorting complete!")