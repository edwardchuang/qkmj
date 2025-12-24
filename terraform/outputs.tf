output "server_ip" {
  description = "The public IP address of the QKMJ server"
  value       = google_compute_address.static_ip.address
}

output "artifact_registry_repo" {
  description = "The Artifact Registry repository name"
  value       = google_artifact_registry_repository.qkmj_repo.name
}