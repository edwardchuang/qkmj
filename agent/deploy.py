import os
import sys
import vertexai
from vertexai import agent_engines
from vertexai.agent_engines import AdkApp

# Add project root to path for app imports
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.dirname(SCRIPT_DIR)
sys.path.insert(0, PROJECT_DIR)

class GlobalAdkApp(AdkApp):
    def set_up(self) -> None:
        vertexai.init()
        super().set_up()
        os.environ["GOOGLE_CLOUD_LOCATION"] = 'global' # overridding the location for Gemini 3
        os.environ["GOOGLE_GENAI_USE_VERTEXAI"] = "TRUE"

def cleanup_old_agents(display_name: str, keep_count: int = 3):
    """Lists agent engines with the given display name and deletes all but the most recent keep_count."""
    print(f"\n🧹 Cleaning up old agents (keeping most recent {keep_count})...")
    try:
        # List all agent engines in the current project/location
        engines = agent_engines.list()
        
        # Filter by display name
        target_engines = [e for e in engines if e.display_name == display_name]
        
        if len(target_engines) <= keep_count:
            print(f"Found {len(target_engines)} agents. No cleanup needed.")
            return

        # Sort by creation time (newest first)
        # Note: Resource object usually has create_time
        target_engines.sort(key=lambda x: x.create_time, reverse=True)

        to_delete = target_engines[keep_count:]
        print(f"Found {len(target_engines)} agents. Deleting {len(to_delete)} old versions...")

        for engine in to_delete:
            print(f"Deleting old agent: {engine.resource_name.split('/')[-1]} (Created: {engine.create_time})")
            try:
                # Force delete to remove child resources (like sessions)
                agent_engines.delete(resource_name=engine.resource_name, force=True)
                #print(f"Deleted resource name: {engine.resource_name.split('/')[-1]}")
            except Exception as e:
                print(f"Failed to delete {engine.resource_name}: {e}")

    except Exception as e:
        print(f"Error during cleanup: {e}")

def main():
    print("\nDeploying...")
    try:
        from agent import root_agent
        app = GlobalAdkApp(
            agent=root_agent,
            enable_tracing=True,
        )
    except ImportError:
        raise

    try:
        import vertexai
        from vertexai import agent_engines

        project_id = os.environ.get("GCP_PROJECT_ID", "qkmj-ai")
        location = os.environ.get("GCP_REGION", "us-central1")
        staging_bucket = os.environ.get("GCS_STAGING_BUCKET", "gs://qkmj_ai_agent_engine_staging")

        print(f"Initializing Vertex AI with Project: {project_id}, Location: {location}, Staging Bucket: {staging_bucket}")

        vertexai.init(
            project=project_id,
            location=location,
            staging_bucket=staging_bucket
        )

        requirements = [
            "google-cloud-aiplatform[adk,agent_engines]>=1.111",
            "google-adk>=1.21.0",
            "google-genai>=1.55.0",
            "pydantic==2.12.5", 
            "cloudpickle==3.1.2",
        ]

        env_vars = {
            "GOOGLE_GENAI_USE_VERTEXAI": "TRUE",  # Use Vertex AI (not AI Studio)
            "GEMINI_MODEL_LOCATION": "global",  # For Gemini 3 models
        }

        # Use standard module-level agent_engines.create()
        # extra_packages bundles the local app/ directory so remote has access to app.tools, etc.
        remote_app = agent_engines.create(
            agent_engine=app,
            requirements=requirements,
            env_vars=env_vars,  # Pass env vars including GEMINI_MODEL_LOCATION
            display_name="QK Agent",
            description="QKMJ Mahjong Agent, My name is Agent Q",
        )
    except Exception as e:
        print(f"\nDeployment failed: {e}")
        return 1

    print()
    print(f"Resource Name: {remote_app.resource_name}")
    
    # Construct the execution URL
    # Format: https://<location>-aiplatform.googleapis.com/v1/projects/<project>/locations/<location>/agentEngines/<agent_id>:query
    query_url = f"https://{location}-aiplatform.googleapis.com/v1/{remote_app.resource_name}:query"
    print(f"Query URL: {query_url}")

    # Cleanup old agent engines
    cleanup_old_agents(display_name="QK Agent")
    return 0

if __name__ == "__main__":
    sys.exit(main())