from statistics import mode
from telnetlib import STATUS
from django.db import models
from datetime import datetime
from django import forms
from django.contrib.auth.forms import UserCreationForm
from django.contrib.auth.models import User

# Create your models here.

class CreatedUsersForm(UserCreationForm):
   email = forms.EmailField(error_messages={'required':'email should not be empty'})

   class Meta:
      model = User
      fields = ['username', 'password1', 'password2', 'email']
   
      
class Truck(models.Model):
   truckid = models.IntegerField(default=0, primary_key=True)
   truckx = models.DecimalField(max_digits=6, decimal_places=2, null=True)
   trucky = models.DecimalField(max_digits=6, decimal_places=2, null=True)
   status = models.IntegerField(default=0, null=True)
   
class Package(models.Model):
    packageid = models.IntegerField(default=0, primary_key=True)
    warehouseid = models.IntegerField(default=0, null=True)
    userx = models.DecimalField(max_digits=6, decimal_places=2, null=True)
    usery = models.DecimalField(max_digits=6, decimal_places=2, null=True)
    truckid = models.ForeignKey(Truck, on_delete=models.CASCADE, null=True)
    status = models.IntegerField(default=0, null=True)
    finish = models.IntegerField(default=0, null=True)
    change = models.IntegerField(default=0, null=True)
    user = models.ForeignKey(User, on_delete=models.CASCADE, null=True)
    
class Survey(models.Model):
   package = models.ForeignKey(Package, on_delete=models.CASCADE)
   comment = models.CharField(max_length=200)
   user = models.ForeignKey(User, on_delete=models.CASCADE)
   
class Tracked(models.Model):
   package = models.ForeignKey(Package, on_delete=models.CASCADE)
   userx = models.DecimalField(max_digits=6, decimal_places=2, null=True)
   usery = models.DecimalField(max_digits=6, decimal_places=2, null=True)
   status = models.IntegerField(default=0, null=True)
   time = models.DateTimeField(auto_now_add=True)
   user = models.ForeignKey(User, on_delete=models.CASCADE, null=True)