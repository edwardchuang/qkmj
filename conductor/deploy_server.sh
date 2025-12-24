#!/bin/bash
set -e

# Configuration
PROJECT_ID=$(gcloud config get-value project)
INSTANCE_NAME="qkmj-server"
ZONE="asia-east1-a"
REGION="asia-east1"
REPO_NAME="qkmj-repo"
IMAGE_NAME="qkmj-server"

echo "🚀 Starting remote deployment to ${INSTANCE_NAME}..."

# Construct the full image path
IMAGE_PATH="${REGION}-docker.pkg.dev/${PROJECT_ID}/${REPO_NAME}/${IMAGE_NAME}:latest"

# Commands to run on the GCE instance
REMOTE_COMMANDS=$(cat <<EOF
  set -e
  echo "Logging into Artifact Registry..."
  gcloud auth configure-docker ${REGION}-docker.pkg.dev -q

  echo "Pulling latest image: ${IMAGE_PATH}"
  docker pull ${IMAGE_PATH}

  echo "Restarting qkmj-server container..."
  docker stop qkmj-server || true
  docker rm qkmj-server || true

  docker run -d \
    --name qkmj-server \
    --network qkmj-net \
    --restart always \
    -p 7001:7001 \
    -e MONGO_URI="mongodb://mongo:27017" \
    ${IMAGE_PATH}

  echo "✅ Server successfully restarted."
EOF
)

# Execute the commands via SSH
gcloud compute ssh ${INSTANCE_NAME} --zone=${ZONE} --command="${REMOTE_COMMANDS}"

echo "🎉 Deployment to GCE complete!"

