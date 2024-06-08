# Use the latest TensorFlow image with Jupyter support
FROM tensorflow/tensorflow:2.17.0-jupyter

# Set the working directory
WORKDIR /app

# Copy the requirements file into the container
COPY requirements.txt .

# Install the required Python packages
RUN pip install --upgrade pip && \
    pip install -r requirements.txt

# Install system dependencies for opencv
RUN apt-get update && apt-get install -y libgl1-mesa-dev

# Copy the current directory contents into the container at /app
COPY . /app

# Expose port 8888 for Jupyter Notebook
EXPOSE 8888

# Run Jupyter Notebook
CMD ["jupyter", "notebook", "--ip=0.0.0.0", "--port=8888", "--no-browser", "--allow-root"]