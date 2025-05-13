package adoptionsystemproject; // Declaring class package

public class AdoptionRecord // Declaring AdoptionRecord class 
{
    private Adopter adopter; // Declaring private adopter variable
    private Animal animal; // Declaring private animal variable

    public AdoptionRecord(Adopter adopter, Animal animal) // Declaring adoption record variables 
    {
        this.adopter = adopter; // Declaring this operation to adopter variable
        this.animal = animal; // Declaring this operation to animal variable
    }

    public Adopter getAdopter() // Declaring adopter option 
    {
        return adopter; // Declaring adopter to getAdopter variable
    }

    public Animal getAnimal() // Declaring animal option 
    {
        return animal; // Declaring animal to getAnimal variable
    }
}
