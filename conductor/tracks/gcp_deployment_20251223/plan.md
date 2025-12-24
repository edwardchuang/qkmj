# Plan: GCP Cloud Deployment & CI/CD Integration

## Phase 1: Artifact Registry & GitHub Actions Setup
**Goal:** Establish the foundation for container image management and automated builds.

- [x] Task: Create Google Artifact Registry via Terraform. 1983a8a
- [x] Task: Configure GitHub Actions Secrets for GCP authentication. 04592d0
- [x] Task: Implement CI workflow to build and test C code. 04592d0
- [x] Task: Implement CD workflow to build and push Docker images to Artifact Registry. 04592d0
- [ ] Task: Conductor - User Manual Verification 'Phase 1: Artifact Registry & GitHub Actions Setup' (Protocol in workflow.md)

## Phase 2: Server Deployment (GCE)
**Goal:** Provision and deploy the `mjgps` server to Google Compute Engine.

- [x] Task: Finalize Terraform for GCE instance and networking (firewall). de1b633
- [x] Task: Implement deployment script/automation to pull and run the server container on GCE. de1b633
- [x] Task: Verify server connectivity and MongoDB integration on GCE. de1b633
- [ ] Task: Conductor - User Manual Verification 'Phase 2: Server Deployment (GCE)' (Protocol in workflow.md)

## Phase 3: Client Deployment (Cloud Run)
**Goal:** Deploy the `qkmj` client to Google Cloud Run.

- [ ] Task: Finalize Terraform for Cloud Run service.
- [ ] Task: Implement deployment automation for Cloud Run.
- [ ] Task: Verify client connectivity to the GCE server and AI backend.
- [ ] Task: Conductor - User Manual Verification 'Phase 3: Client Deployment (Cloud Run)' (Protocol in workflow.md)

## Phase 4: AI Agent Deployment (Agent Engine)
**Goal:** Deploy the Python AI Agent to GCP Vertex AI Agent Engine.

- [~] Task: Implement CI workflow for Python AI Agent (testing/linting). de1b633
- [~] Task: Implement CD workflow for Python AI Agent (deployment to Agent Engine). de1b633
- [ ] Task: Verify Agent Engine connectivity and model response.
- [ ] Task: Conductor - User Manual Verification 'Phase 4: AI Agent Deployment (Agent Engine)' (Protocol in workflow.md)
