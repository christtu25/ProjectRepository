package adoptionsystemproject; // Declaring class package

public class Adopter // Declaring Adopter class operations
{
    private String adopter_ID; // Declaring ID variable
    private String name; // Declaring name variable
    private int age; // Declaring age variable
    private String preferredPet; // Declaring desired pet variable

    public Adopter(String adopter_ID, String name, int age, String preferredPet) // Declaring constructor for this operation to attribute variables 
    {
        this.adopter_ID = adopter_ID; // Declaring this operation to adopter_ID variable
        this.name = name; // Declaring this operation to name variable
        this.age = age; // Declaring this operation to age variable
        this.preferredPet = preferredPet; // Declaring this operation to preferredPet variable
    }

    // Getters
    public String getAdopter_ID() 
    {
        return adopter_ID;
    }

    public String getName() 
    {
        return name;
    }

    public int getAge() 
    {
        return age;
    }

    public String getPreferredPet() 
    {
        return preferredPet;
    }

    // Setters
    public void setAdopter_ID(String adopter_ID) 
    {
        this.adopter_ID = adopter_ID;
    }

    public void setName(String name) 
    {
        this.name = name;
    }

    public void setAge(int age) 
    {
        this.age = age;
    }

    public void setPreferredPet(String preferredPet) 
    {
        this.preferredPet = preferredPet;
    }

    public void requestAdoption(Animal animal) throws AdoptionException // Declaring throw operation to AdoptionException class
    {
        if (animal.AdoptionStatus()) // Declaring if statement to status variable
        {
            throw new AdoptionException("This animal is already adopted."); // Declaring new throw print
        }
        
        else // Declaring else statement for successful prompt
        {
            animal.setAdopted(true); // Declaring picked animal to true status
            
            // Additional logic for processing the adoption can be added here :)
            
            System.out.println("Adoption request for " + animal.getName() + " is complete."); // Printing user prompt for successful adoption
        }
    }
}
