import torch
import argparse
from models.experimental import attempt_load

def export(weights, output):
    # Load model
    device = torch.device("cpu")
    model = attempt_load(weights, device)
    model.eval()

    # Save as TorchScript
    torch.jit.save(torch.jit.script(model), output)
    print(f"Model exported to {output}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--weights", type=str, required=True, help="Path to .pt file")
    parser.add_argument("--output", type=str, default="best_exported.pt", help="Output .pt file")
    args = parser.parse_args()
    export(args.weights, args.output)