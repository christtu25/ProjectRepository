package adoptionsystemproject; // Declaring class package

public abstract class Animal implements Adoptable // Declaring Animal class implementing Adoptable interface
{
    protected String animal_ID; // Declaring ID variable
    protected String name; // Declaring name variable
    protected int age; // Declaring age variable
    protected String gender; // Declaring gender variable
    protected boolean isAdopted; // Declaring status variable

    public Animal(String animal_ID, String name, int age, String gender) // Declaring animal class from attribute variables 
    {
        this.animal_ID = animal_ID; // Declaring this operation to ID variable
        this.name = name; // Declaring this operation to name variable
        this.age = age; // Declaring this operation to age variable
        this.gender = gender; // Declaring this operation to gender variable
        this.isAdopted = false; // Declaring this operation to status variable defaulted to as false
    }

    @Override // Declaring system to override
    public boolean AdoptionStatus() // Declaring to implement Adoptable interface methods 
    {
        return isAdopted; // Declaring status variable to return operation
    }

    @Override // Declaring system to override
    public abstract void prepareAdoption(); // Declaring abstract method to implement in subclasses

    // Getters
    public String getAnimal_ID() 
    {
        return animal_ID;
    }

    public String getName() 
    {
        return name;
    }

    public int getAge() 
    {
        return age;
    }

    public String getGender() 
    {
        return gender;
    }

    // Setters
    public void setAnimal_ID(String animal_ID) 
    {
        this.animal_ID = animal_ID;
    }

    public void setName(String name) 
    {
        this.name = name;
    }

    public void setAge(int age) 
    {
        this.age = age;
    }

    public void setGender(String gender) 
    {
        this.gender = gender;
    }

    public void setAdopted(boolean adopted) 
    {
        isAdopted = adopted;
    }

    public String getDetails() 
    {
        return "ID: " + animal_ID + ", Name: " + name + ", Age: " + age + ", Gender: " + gender + ", Adopted: " + (isAdopted ? "Yes" : "No");
    }
}
