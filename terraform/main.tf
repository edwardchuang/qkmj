provider "google" {
  project = var.project_id
  region  = var.region
  zone    = var.zone
}

# 1. Firewall Rule: Allow Game Port (7001)
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

# 2. Service Account for the VM
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

# 3. Artifact Registry Repository
resource "google_artifact_registry_repository" "qkmj_repo" {
  location      = var.region
  repository_id = "qkmj-repo"
  description   = "Docker repository for QKMJ images"
  format        = "DOCKER"
}

# 4. The GCE Instance
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
    # access_config {} removed for internal-only IP
  }

  service_account {
    email  = google_service_account.qkmj_sa.email
    scopes = ["cloud-platform"]
  }

  # Startup Script: Run Game Server
  metadata_startup_script = <<-EOT
    #! /bin/bash
    set -e
    
    # Configure Docker Auth for Artifact Registry
    gcloud auth configure-docker ${var.region}-docker.pkg.dev -q
    
    # Create Docker Network
    docker network create qkmj-net || true
      
    # 1. Start QKMJ Server
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
      -e MONGO_URI="${var.mongo_uri}" \
      ${var.region}-docker.pkg.dev/${var.project_id}/qkmj-repo/qkmj-server:latest
      
    echo "Deployment Complete."
  EOT
}

# 5. Serverless VPC Access Connector (to reach GCE Internal IP)
resource "google_vpc_access_connector" "connector" {
  name          = "qkmj-vpc-connector"
  ip_cidr_range = "10.8.0.0/28"
  network       = "default"
  region        = var.region
}

# 6. Cloud Run Service (The Client)
resource "google_cloud_run_v2_service" "qkmj_client" {
  name     = "qkmj-client"
  location = var.region
  ingress  = "INGRESS_TRAFFIC_ALL"

  template {
    vpc_access {
      connector = google_vpc_access_connector.connector.id
      egress    = "ALL_TRAFFIC"
    }

    containers {
      image = "${var.region}-docker.pkg.dev/${var.project_id}/qkmj-repo/qkmj-client:latest"
      
      ports {
        container_port = 8080
      }

      env {
        name  = "SERVER_IP"
        value = google_compute_instance.qkmj_server.network_interface.0.network_ip
      }
      env {
        name  = "SERVER_PORT"
        value = "7001"
      }
    }
  }

  depends_on = [google_compute_instance.qkmj_server]
}
