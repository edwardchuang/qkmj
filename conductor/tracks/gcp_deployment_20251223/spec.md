# Spec: GCP Cloud Deployment & CI/CD Integration

## Goal
Establish a fully automated CI/CD pipeline and infrastructure for the QKMJ system on Google Cloud Platform. This ensures that every change to the codebase is automatically built, tested, and deployed to a stable environment.

## Objectives
- Automate the build and containerization of the C client (`qkmj`) and C server (`mjgps`).
- Implement a CI pipeline using GitHub Actions for automated testing and image creation.
- Set up Google Artifact Registry to host the Docker images.
- Provision and configure Google Compute Engine (GCE) for the server.
- Provision and configure Google Cloud Run for the client.
- Finalize Terraform configurations for reproducible infrastructure.

## Requirements

### CI/CD (GitHub Actions)
- Trigger on every push to the `main` branch.
- Build the C components using CMake.
- Execute the existing GoogleTest suite.
- Build Docker images using the provided `Dockerfile.client` and `Dockerfile.server`.
- Authenticate with GCP and push images to Google Artifact Registry.

### Infrastructure (Terraform)
- **Artifact Registry:** Create a repository to store the `qkmj-client` and `qkmj-server` images.
- **Compute Engine (Server):** 
  - Provision a VM instance to run the `mjgps` server.
  - Configure network firewall rules to allow traffic on port 7001 (and any other necessary ports).
  - Ensure the VM has access to the Artifact Registry to pull the server image.
- **Cloud Run (Client):**
  - Deploy the `qkmj` client container.
  - Configure environment variables (`AI_ENDPOINT`, etc.) as needed.

### Deployment Logic
- The server should automatically pull and run the latest image from the Artifact Registry upon deployment.
- The client should be accessible via a Cloud Run URL.

## Success Criteria
- [ ] GitHub Actions pipeline completes successfully for both client and server.
- [ ] Docker images are present in the Google Artifact Registry.
- [ ] Terraform successfully provisions all required resources.
- [ ] The `mjgps` server is reachable on GCE.
- [ ] The `qkmj` client is functional on Cloud Run.
