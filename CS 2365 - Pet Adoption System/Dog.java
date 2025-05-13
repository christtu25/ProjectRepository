package adoptionsystemproject; // Declaring class package

public class Dog extends Animal // Declaring dog class extending Animal class
{
    private boolean isIndoor; // Declaring private boolean isIndoor variable

    public Dog(String animal_ID, String name, int age, String gender, boolean isIndoor) // Declaring dog class attribute variables 
    {
        super(animal_ID, name, age, gender); // Declaring super operation to attribute variables
        this.isIndoor = isIndoor; // Declaring this operation to isIndoor variable
    }

    @Override // Declaring system to override
    public void prepareAdoption() // Declaring implementation to the abstract method from Animal
    {
        // Implementation for preparing a dog for adoption...
    }

    // Getters
    public boolean isIndoor() 
    {
        return isIndoor;
    }

    // Setters
    public void setIndoor(boolean isIndoor) 
     {
        this.isIndoor = isIndoor;
    }

    // Additional methods specific to dog can be added here
}
