provider "google" {
  project = var.project_id
  region  = var.region
  zone    = var.zone
}

# 1. Static IP Address for the Game Server
resource "google_compute_address" "static_ip" {
  name = "qkmj-server-ip"
}

# 2. Firewall Rule: Allow Game Port (7001)
resource "google_compute_firewall" "allow_qkmj" {
  name    = "allow-qkmj-server"
  network = "default"

  allow {
    protocol = "tcp"
    ports    = ["7001"]
  }

  source_ranges = ["0.0.0.0/0"]
  target_tags   = ["qkmj-server"]
}

# 3. Service Account for the VM
resource "google_service_account" "qkmj_sa" {
  account_id   = "qkmj-server-sa"
  display_name = "QKMJ Server Service Account"
}

# Allow SA to pull images from Artifact Registry
resource "google_project_iam_member" "artifact_registry_reader" {
  project = var.project_id
  role    = "roles/artifactregistry.reader"
  member  = "serviceAccount:${google_service_account.qkmj_sa.email}"
}

# 4. Artifact Registry Repository
resource "google_artifact_registry_repository" "qkmj_repo" {
  location      = var.region
  repository_id = "qkmj-repo"
  description   = "Docker repository for QKMJ images"
  format        = "DOCKER"
}

# 5. The GCE Instance
resource "google_compute_instance" "qkmj_server" {
  name         = "qkmj-server"
  machine_type = "e2-medium" # Cost-effective but capable
  tags         = ["qkmj-server", "http-server"]

  boot_disk {
    initialize_params {
      image = "cos-cloud/cos-stable" # Container-Optimized OS
    }
  }

  network_interface {
    network = "default"
    access_config {
      nat_ip = google_compute_address.static_ip.address
    }
  }

  service_account {
    email  = google_service_account.qkmj_sa.email
    scopes = ["cloud-platform"]
  }

  # Startup Script: Run Mongo and Game Server
  metadata_startup_script = """
    #! /bin/bash
    set -e
    
    # Configure Docker Auth for Artifact Registry
    gcloud auth configure-docker ${var.region}-docker.pkg.dev -q
    
    # Create Docker Network
    docker network create qkmj-net || true
    
    # 1. Start MongoDB
    echo "Starting MongoDB..."
    docker run -d \
      --name mongo \
      --network qkmj-net \
      --restart always \
      -v /var/lib/mongo:/data/db \
      mongo:5.0
      
    # 2. Start QKMJ Server
    echo "Starting QKMJ Server..."
    # Remove old container if exists (for updates)
    docker stop qkmj-server || true
    docker rm qkmj-server || true
    
    # Pull latest image from Artifact Registry
    docker pull ${var.region}-docker.pkg.dev/${var.project_id}/qkmj-repo/qkmj-server:latest
    
    # Run Server
    docker run -d \
      --name qkmj-server \
      --network qkmj-net \
      --restart always \
      -p 7001:7001 \
      -e MONGO_URI="mongodb://mongo:27017" \
      ${var.region}-docker.pkg.dev/${var.project_id}/qkmj-repo/qkmj-server:latest
      
    echo "Deployment Complete."
  """
}
