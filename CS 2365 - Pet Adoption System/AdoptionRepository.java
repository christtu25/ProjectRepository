package adoptionsystemproject; // Declare class package

import java.util.ArrayList; // Declaring class array operations
import java.util.List; // Declaring class list operations

class AdoptionRepository // Declaring AdoptionRepository class  
{
    private List<AdoptionRecord> completedAdoptions = new ArrayList<>(); // Declaring completed adoption array

    public void addAdoption(Adopter adopter, Animal animal) // Declaring adopter to animal relationship
    {
        completedAdoptions.add(new AdoptionRecord(adopter, animal)); // Declaring completed adoption to record array
    }

    public List<AdoptionRecord> getCompletedAdoptions() // Declaring adoption array to completed adoptions 
    {
        return completedAdoptions; // Declaring completed adoptions list to CompletedAdoptions list
    }
}
