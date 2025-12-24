variable "project_id" {
  description = "The Google Cloud Project ID"
  type        = string
}

variable "region" {
  description = "GCP Region"
  type        = string
  default     = "asia-east1"
}

variable "zone" {
  description = "GCP Zone"
  type        = string
  default     = "asia-east1-a"
}

variable "mongo_uri" {
  description = "The MongoDB connection string"
  type        = string
  sensitive   = true
}
