output "server_internal_ip" {
  description = "The internal IP address of the QKMJ server"
  value       = google_compute_instance.qkmj_server.network_interface.0.network_ip
}

output "artifact_registry_repo" {
  description = "The Artifact Registry repository name"
  value       = google_artifact_registry_repository.qkmj_repo.name
}
