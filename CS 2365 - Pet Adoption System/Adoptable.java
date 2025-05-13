package adoptionsystemproject; // Declaring class package

public interface Adoptable // Declaring Adoptable interface operations
{
    boolean AdoptionStatus(); // Declaring boolean status operations
    void prepareAdoption(); // Declaring void preparation operations
    
    // Default method (optional)
    default void printAdoptionStatus() 
    {
        System.out.println("Adoption status: " + AdoptionStatus());
    }
}
