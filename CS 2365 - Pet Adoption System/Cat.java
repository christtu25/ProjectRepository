package adoptionsystemproject; // Declaring class package

public class Cat extends Animal // Declaring Cat class extending Animal class
{
    private boolean isIndoor; // Declaring private boolean isIndoor variable

    public Cat(String animal_ID, String name, int age, String gender, boolean isIndoor) // Declaring Cat class attribute variables 
    {
        super(animal_ID, name, age, gender); // Declaring super operation to attribute variables
        this.isIndoor = isIndoor; // Declaring this operation to isIndoor variable
    }

    @Override // Declaring system to override
    public void prepareAdoption() // Declaring implementation to the abstract method from Animal
    {
        // Implementation for preparing a cat for adoption...
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

    // Additional methods specific to Cat can be added here
}
