# GCP and GitHub Actions Setup Guide

To enable automated deployment to Google Cloud Platform, you need to configure authentication between GitHub and GCP. **Workload Identity Federation (WIF)** is the preferred and most secure method as it doesn't require long-lived JSON keys.

---

## Option A: Workload Identity Federation (Preferred)

Use this method to authenticate without storing sensitive JSON keys. This is often required by organization policies.

### 1. Enable Required APIs
```bash
gcloud services enable \
    iamcredentials.googleapis.com \
    artifactregistry.googleapis.com \
    compute.googleapis.com \
    run.googleapis.com
```

### 2. Create Service Account and Assign Roles
```bash
export PROJECT_ID=$(gcloud config get-value project)

gcloud iam service-accounts create github-actions-deploy \
    --display-name="GitHub Actions Deployment Service Account"

# Artifact Registry
gcloud projects add-iam-policy-binding $PROJECT_ID \
    --member="serviceAccount:github-actions-deploy@$PROJECT_ID.iam.gserviceaccount.com" \
    --role="roles/artifactregistry.writer"

# Cloud Run
gcloud projects add-iam-policy-binding $PROJECT_ID \
    --member="serviceAccount:github-actions-deploy@$PROJECT_ID.iam.gserviceaccount.com" \
    --role="roles/run.admin"

# GCE
gcloud projects add-iam-policy-binding $PROJECT_ID \
    --member="serviceAccount:github-actions-deploy@$PROJECT_ID.iam.gserviceaccount.com" \
    --role="roles/compute.admin"

# Service Account User (Needed to deploy Cloud Run)
gcloud projects add-iam-policy-binding $PROJECT_ID \
    --member="serviceAccount:github-actions-deploy@$PROJECT_ID.iam.gserviceaccount.com" \
    --role="roles/iam.serviceAccountUser"
```

### 3. Configure Workload Identity Federation
```bash
export REPO="your-username/qkmj" # e.g. "edwardc/qkmj" 

# Create Pool
gcloud iam workload-identity-pools create "github-pool" \
    --project="${PROJECT_ID}" --location="global"

# Create Provider
gcloud iam workload-identity-pools providers create-oidc "github-provider" \
    --project="${PROJECT_ID}" --location="global" \
    --workload-identity-pool="github-pool" \
    --attribute-mapping="google.subject=assertion.sub,attribute.actor=assertion.actor,attribute.repository=assertion.repository" \
    --issuer-uri="https://token.actions.githubusercontent.com"

# Allow GitHub to impersonate the Service Account
export POOL_ID=$(gcloud iam workload-identity-pools describe "github-pool" \
    --project="${PROJECT_ID}" --location="global" --format='value(name)')

gcloud iam service-accounts add-iam-policy-binding "github-actions-deploy@${PROJECT_ID}.iam.gserviceaccount.com" \
    --project="${PROJECT_ID}" \
    --role="roles/iam.workloadIdentityUser" \
    --member="principalSet://iam.googleapis.com/${POOL_ID}/attribute.repository/${REPO}"
```

### 4. Configure GitHub Secrets
Add the following secrets to your repository (**Settings > Secrets and variables > Actions**):
- `GCP_PROJECT_ID`: Your project ID.
- `GCP_WORKLOAD_IDENTITY_PROVIDER`: The full path to the provider (e.g. `projects/123456789/locations/global/workloadIdentityPools/github-pool/providers/github-provider`).

---

## Option B: Service Account JSON Key (Traditional)

Use this only if Workload Identity Federation is not an option.

1.  **Follow Step 2 above** to create the Service Account and assign roles.
2.  **Generate JSON Key**:
    ```bash
    gcloud iam service-accounts keys create gcp-sa-key.json \
      --iam-account=github-actions-deploy@$PROJECT_ID.iam.gserviceaccount.com
    ```
3.  **GitHub Secret**: Add the content of `gcp-sa-key.json` as the secret **`GCP_SA_KEY`**.
4.  **Clean up**: `rm gcp-sa-key.json`.