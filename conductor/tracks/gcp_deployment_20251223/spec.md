# Spec: GCP Cloud Deployment & CI/CD Integration

## Goal
Establish a fully automated CI/CD pipeline and infrastructure for the QKMJ system on Google Cloud Platform. This ensures that every change to the codebase is automatically built, tested, and deployed to a stable environment.

## Objectives
- Automate the build and containerization of the C client (`qkmj`) and C server (`mjgps`).
- Implement a CI pipeline using GitHub Actions for automated testing and image creation.
- Set up Google Artifact Registry to host the Docker images.
- Provision and configure Google Compute Engine (GCE) for the server (Private Networking).
- Provision and configure Google Cloud Run for the client.
- Deploy and host the Python AI Agent on **GCP Agent Engine (Vertex AI Agents)**.
- Finalize Terraform configurations for reproducible infrastructure.

## Requirements

### CI/CD (GitHub Actions)
- **C Components**: Build with CMake, test with GTest, push to Artifact Registry.
- **Python Agent**: 
  - Run linting and unit tests for the `google-adk` based agent.
  - Automate deployment to Vertex AI Agent Engine using OIDC/WIF.

### Infrastructure (Terraform)
- **Artifact Registry:** Create a repository to store the `qkmj-client` and `qkmj-server` images.
- **Compute Engine (Server):** 
  - Provision a private VM instance (no external IP).
  - Configure network firewall rules to allow traffic on port 7001.
  - Enable Identity-Aware Proxy (IAP) for secure management.
- **Cloud Run (Client):**
  - Deploy the `qkmj` client container.
- **Vertex AI:**
  - Enable necessary APIs for Agent Engine.

### Deployment Logic
- The server should automatically pull and run the latest image from the Artifact Registry upon deployment.
- The AI Agent is hosted as a serverless Vertex AI Agent, accessible via a stable endpoint.

## Success Criteria
- [ ] GitHub Actions pipeline completes successfully for both client and server.
- [ ] Docker images are present in the Google Artifact Registry.
- [ ] Terraform successfully provisions all required resources.
- [ ] The `mjgps` server is reachable on GCE.
- [ ] The `qkmj` client is functional on Cloud Run.
