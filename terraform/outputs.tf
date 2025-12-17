output "server_ip" {
  description = "The public static IP address of the game server"
  value       = google_compute_address.static_ip.address
}
