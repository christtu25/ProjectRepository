import torch

pt_file = "best.pt"  # Update if named differently, e.g., best_v6.pt

checkpoint = torch.load(pt_file, map_location='cpu')
model = checkpoint['model']
class_names = model.names  # List of class names

# Print class names
print("Class names:", class_names)